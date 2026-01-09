#include "AudioProcess.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <cmath> // abs 需要这个

// ==========================================
// 构造与析构
// ==========================================

// [修复] 参数改名，避免与成员变量重名 (解决 Shadow 警告)
AudioProcess::AudioProcess(int sr, int ch, int duration_ms)
    : sample_rate(sr), 
      channels(ch), 
      frame_duration_ms(duration_ms),
      isRunning(false),
      pcm_in(nullptr),
      pcm_out(nullptr) {

    // [硬件初始化] 静默设置音量
    system("amixer set 'DAC LINEOUT' 20 > /dev/null 2>&1");
    printf(">>> [Audio] Service Init (Vol: 20)\n");
    
    // 计算 buffer 大小
    period_size = (sample_rate * frame_duration_ms) / 1000;
    // 确保是 32 的倍数 (TinyALSA 某些版本的要求)
    period_size = (period_size / 32) * 32;
}

AudioProcess::~AudioProcess() {
    stop();
}

// ==========================================
// 核心控制接口
// ==========================================

bool AudioProcess::start() {
    if (isRunning) return true;

    isRunning = true;
    printf("[Audio] Starting threads...\n");

    recordThread = std::thread(&AudioProcess::recordLoop, this);
    playThread = std::thread(&AudioProcess::playLoop, this);

    return true;
}

void AudioProcess::stop() {
    if (!isRunning) return;
    
    printf("[Audio] Stopping...\n");
    isRunning = false;

    playbackCV.notify_all(); // 唤醒播放线程

    if (recordThread.joinable()) recordThread.join();
    if (playThread.joinable()) playThread.join();

    if (pcm_in) {
        pcm_close(pcm_in);
        pcm_in = NULL;
    }
    if (pcm_out) {
        pcm_close(pcm_out);
        pcm_out = NULL;
    }
    printf("[Audio] Stopped.\n");
}

// ==========================================
// 数据接口
// ==========================================

bool AudioProcess::getRecordedAudio(std::vector<int16_t>& recordedData) {
    std::lock_guard<std::mutex> lock(recordMutex);
    if (recordedQueue.empty()) {
        return false;
    }
    recordedData = recordedQueue.front();
    recordedQueue.pop();
    return true;
}

void AudioProcess::addFrameToPlaybackQueue(const std::vector<int16_t>& pcm_frame) {
    {
        std::lock_guard<std::mutex> lock(playbackMutex);
        playbackQueue.push(pcm_frame);
        
        // 防止堆积太多导致延迟
        if (playbackQueue.size() > 10) {
            playbackQueue.pop(); 
        }
    }
    playbackCV.notify_one();
}

// ==========================================
// 线程循环
// ==========================================

void AudioProcess::recordLoop() {
    struct pcm_config config;
    memset(&config, 0, sizeof(config));
    config.channels = channels;
    config.rate = sample_rate;
    config.period_size = period_size;
    config.period_count = 4; // 减小 buffer 数量以降低延迟
    config.format = PCM_FORMAT_S16_LE;

    printf("[Audio] Opening Capture (0,0)...\n");
    pcm_in = pcm_open(0, 0, PCM_IN, &config);

    if (!pcm_in || !pcm_is_ready(pcm_in)) {
        printf("[Audio] Error open Capture: %s\n", pcm_get_error(pcm_in));
        return;
    }

    int buffer_size = pcm_frames_to_bytes(pcm_in, config.period_size);
    std::vector<int16_t> buffer(period_size * channels);

    while (isRunning) {
        int ret = pcm_read(pcm_in, buffer.data(), buffer_size);
        
        if (ret == 0) { // 0 表示成功 (新版 tinyalsa 可能是 0，旧版可能是 >=0)
            std::lock_guard<std::mutex> lock(recordMutex);
            recordedQueue.push(buffer);
            recordCV.notify_one();
        } else {
            // 只有出错才打印，防止刷屏
            // printf("[Audio] Read error: %s\n", pcm_get_error(pcm_in));
            usleep(5000); 
        }
    }
}

void AudioProcess::playLoop() {
    struct pcm_config config;
    memset(&config, 0, sizeof(config));
    config.channels = channels;
    config.rate = sample_rate;
    config.period_size = period_size;
    config.period_count = 4;
    config.format = PCM_FORMAT_S16_LE;

    printf("[Audio] Opening Playback (0,0)...\n");
    pcm_out = pcm_open(0, 0, PCM_OUT, &config);

    if (!pcm_out || !pcm_is_ready(pcm_out)) {
        printf("[Audio] Error open Playback: %s\n", pcm_get_error(pcm_out));
        return;
    }

    int buffer_size = pcm_frames_to_bytes(pcm_out, config.period_size);

    while (isRunning) {
        std::vector<int16_t> frame;
        
        {
            std::unique_lock<std::mutex> lock(playbackMutex);
            playbackCV.wait_for(lock, std::chrono::milliseconds(50), [this] {
                return !playbackQueue.empty() || !isRunning;
            });

            if (!isRunning) break;

            if (playbackQueue.empty()) {
                continue; 
            }

            frame = playbackQueue.front();
            playbackQueue.pop();
        }

        int ret = pcm_write(pcm_out, frame.data(), buffer_size);
        if (ret < 0) {
            printf("[Audio] Write error: %s\n", pcm_get_error(pcm_out));
        }
    }
}

void AudioProcess::playWavFile(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        printf("[Audio] Error: File not found %s\n", filename);
        return;
    }

    printf("[Audio] Playing: %s\n", filename);
    fseek(fp, 44, SEEK_SET); // 跳过头

    std::vector<int16_t> chunk(period_size * channels); 

    while (!feof(fp) && isRunning) {
        size_t read_count = fread(chunk.data(), sizeof(int16_t), chunk.size(), fp);
        
        if (read_count > 0) {
            if (read_count < chunk.size()) {
                std::fill(chunk.begin() + read_count, chunk.end(), 0);
            }

            addFrameToPlaybackQueue(chunk);
            
            // 流控：防止读文件太快把内存撑爆
            while (isRunning) {
                size_t queueSize = 0;
                {
                    std::lock_guard<std::mutex> lock(playbackMutex);
                    queueSize = playbackQueue.size();
                }
                if (queueSize > 8) usleep(10000); // 只要积压 > 8 帧就等等
                else break;
            }
        }
    }

    fclose(fp);
}
