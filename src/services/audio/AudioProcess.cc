/**
 * @file audio_process.h / .cc
 * @brief 音频服务模块 (Audio Service Module)
 * * 基于 TinyALSA 实现的异步音频处理服务。
 * 采用 "生产者-消费者" 模型，通过独立的后台线程处理录音和播放，
 * 确保主线程 (UI/状态机) 不会因为音频 I/O 而阻塞。
 * * 核心功能:
 * 1. RecordLoop: 持续从麦克风读取 PCM 数据存入队列 (供 Snowboy/ASR 使用)。
 * 2. PlayLoop: 从播放队列取出 PCM 数据写入扬声器 (用于 TTS/音效)。
 * 3. 线程安全: 使用 std::mutex 和 std::condition_variable 保护数据队列。
 * @date 2026-01-16
 */

#include "AudioProcess.h" 
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <algorithm> // for std::fill
#include <cstring>

// 定义 WAV 文件头偏移量 (44字节)
#define WAV_HEADER_SIZE 44

struct WavHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t overall_size;      // 文件总大小 - 8
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt_chunk_marker[4] = {'f', 'm', 't', ' '};
    uint32_t length_of_fmt = 16;
    uint16_t format_type = 1;   // 1 = PCM
    uint16_t channels = 1;      // 单声道 (注意：我们要存的是单声道)
    uint32_t sample_rate = 16000;
    uint32_t byterate = 16000 * 16 * 1 / 8; // rate * bits * ch / 8
    uint16_t block_align = 16 * 1 / 8;      // bits * ch / 8
    uint16_t bits_per_sample = 16;
    char data_chunk_header[4] = {'d', 'a', 't', 'a'};
    uint32_t data_size;         // 纯音频数据大小
};

// ==========================================
// 构造与析构
// ==========================================

AudioProcess::AudioProcess() {
    // 初始化 PCM 配置
    // Echo-Mate 硬件需求: 16kHz, 1ch, 16bit
    memset(&config_, 0, sizeof(config_));
    config_.channels = 2;
    config_.rate = 16000;
    config_.period_size = 1024; // 每次处理约 64ms 数据 (1024/16000)
    config_.period_count = 4;   // 缓冲区数量
    config_.format = PCM_FORMAT_S16_LE;
    
    // 初始化静音 (防止爆音)
    system("amixer set 'DAC LINEOUT' 20 > /dev/null 2>&1");
    std::cout << "[Audio] Service Constructed (Rate: 16000, Ch: 1)" << std::endl;
}

AudioProcess::~AudioProcess() {
    Stop();
}

// ==========================================
// 系统控制接口
// ==========================================

bool AudioProcess::Init() {
    // 可以在这里做额外的硬件检查，目前直接返回 true
    return true;
}

bool AudioProcess::Start() {
    if (is_running_.load()) return true;

    is_running_.store(true);
    std::cout << "[Audio] Starting background threads..." << std::endl;

    // 启动录音和播放线程
    record_thread_ = std::thread(&AudioProcess::RecordLoop, this);
    play_thread_ = std::thread(&AudioProcess::PlayLoop, this);

    return true;
}

void AudioProcess::Stop() {
    if (!is_running_.load()) return;

    std::cout << "[Audio] Stopping..." << std::endl;
    is_running_.store(false);

    // 唤醒播放线程以便它能退出等待
    playback_cv_.notify_all();

    if (record_thread_.joinable()) record_thread_.join();
    if (play_thread_.joinable()) play_thread_.join();

    // 关闭 PCM 句柄
    if (pcm_in_) {
        pcm_close(pcm_in_);
        pcm_in_ = nullptr;
    }
    if (pcm_out_) {
        pcm_close(pcm_out_);
        pcm_out_ = nullptr;
    }
    std::cout << "[Audio] Stopped." << std::endl;
}

// ==========================================
// 录音数据接口 (Consumer: IdleState)
// ==========================================

bool AudioProcess::GetFrame(std::vector<int16_t>& chunk) {
    std::lock_guard<std::mutex> lock(record_mutex_);
    
    if (recorded_queue_.empty()) {
        return false;
    }

    // 移动语义优化性能
    chunk = std::move(recorded_queue_.front());
    recorded_queue_.pop();
    
    // 防止队列无限增长 (比如处理太慢时，丢弃旧数据)
    if (recorded_queue_.size() > 50) {
        printf("[Audio] Warning: Recording queue overflow, dropping frames!\n");
        while (recorded_queue_.size() > 10) {
            recorded_queue_.pop();
        }
    }
    
    return true;
}

void AudioProcess::ClearBuff() {
    std::lock_guard<std::mutex> lock(record_mutex_);
    std::queue<std::vector<int16_t>> empty;
    std::swap(recorded_queue_, empty); // 高效清空
    std::cout << "[Audio] Recorded buffer cleared." << std::endl;
}

// ==========================================
// 播放接口 (Producer: SpeakingState)
// ==========================================

void AudioProcess::PutFrame(const std::vector<int16_t>& pcm_frame) {
    {
        std::lock_guard<std::mutex> lock(playback_mutex_);
        playback_queue_.push(pcm_frame);
        
        // 简单的流控：如果积压太多，丢弃旧的？或者在这里阻塞？
        // 语音助手场景通常不允许丢弃，所以不做丢弃，但可以打印警告
        if (playback_queue_.size() > 200) {
            // printf("[Audio] Warning: Playback queue high load!\n");
        }
    }
    playback_cv_.notify_one();
}

void AudioProcess::PlayWavFile(const std::string& filename) {
    FILE* fp = fopen(filename.c_str(), "rb");
    if (!fp) {
        std::cerr << "[Audio] Error: File not found " << filename << std::endl;
        return;
    }

    std::cout << "[Audio] Playing: " << filename << std::endl;
    
    // 跳过 WAV 头
    fseek(fp, WAV_HEADER_SIZE, SEEK_SET);

    // 计算一个周期的数据量 (以 int16 为单位)
    size_t chunk_size = config_.period_size * config_.channels;
    std::vector<int16_t> chunk(chunk_size);

    while (is_running_.load() && !feof(fp)) {
        size_t read_count = fread(chunk.data(), sizeof(int16_t), chunk_size, fp);
        
        if (read_count > 0) {
            // 如果读不够一个周期，补零
            if (read_count < chunk_size) {
                std::fill(chunk.begin() + read_count, chunk.end(), 0);
            }

            PutFrame(chunk);
            
            // [简单流控] 防止读文件太快把内存撑爆
            // 如果队列里积压了超过 0.5秒 的数据 (比如 8 个周期)，就稍微等一下
            while (is_running_.load()) {
                size_t queue_size = 0;
                {
                    std::lock_guard<std::mutex> lock(playback_mutex_);
                    queue_size = playback_queue_.size();
                }
                if (queue_size > 8) {
                    usleep(10000); // 10ms
                } else {
                    break;
                }
            }
        }
    }

    fclose(fp);
}

void AudioProcess::SaveStart(const std::string& filename) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    if (record_fp_) fclose(record_fp_);

    record_fp_ = fopen(filename.c_str(), "wb");
    if (!record_fp_) {
        printf("[Audio] Error: Cannot create file %s\n", filename.c_str());
        return;
    }

    // [关键优化] 先写入 44 字节的空数据占位
    // 这样我们就可以直接往后追加音频，最后再回来填坑
    WavHeader dummy_header;
    memset(&dummy_header, 0, sizeof(dummy_header));
    fwrite(&dummy_header, sizeof(WavHeader), 1, record_fp_);

    printf("[Audio] Start saving to: %s\n", filename.c_str());
}

void AudioProcess::SaveStop() {
    std::lock_guard<std::mutex> lock(file_mutex_);
    if (!record_fp_) return;

    // 1. 获取文件总大小
    fseek(record_fp_, 0, SEEK_END);
    long file_size = ftell(record_fp_);
    
    // 2. 计算数据大小 (总大小 - 44字节头)
    long data_size = file_size - sizeof(WavHeader);
    if (data_size < 0) data_size = 0;

    // 3. 回到文件开头，准备重写头
    fseek(record_fp_, 0, SEEK_SET);

    // 4. 填充真正的 WAV 头
    WavHeader header;
    header.data_size = (uint32_t)data_size;
    header.overall_size = header.data_size + 36;
    
    // 5. 覆盖写入头
    fwrite(&header, sizeof(WavHeader), 1, record_fp_);

    // 6. 关闭文件
    fclose(record_fp_);
    record_fp_ = nullptr;
    
    printf("[Audio] Recording saved. (Size: %ld bytes)\n", file_size);
}

// ==========================================
// 线程循环实现
// ==========================================

void AudioProcess::RecordLoop() {
    printf("[Audio] Capture Thread Started (Hardware: 2ch -> Software: 1ch).\n");
    
    // [适配 RV1106] 硬件必须开启双声道，否则报 Error -22
    config_.channels = 2;

    // 打开录音设备 (card 0, device 0)
    pcm_in_ = pcm_open(0, 0, PCM_IN, &config_);

    if (!pcm_in_ || !pcm_is_ready(pcm_in_)) {
        printf("[Audio] Error opening Capture: %s\n", pcm_get_error(pcm_in_));
        if (pcm_in_) pcm_close(pcm_in_);
        return;
    }

    // 计算 buffer 大小
    // 注意：因为硬件是双声道，所以读取的帧数对应的字节数是单声道的2倍
    int stereo_frame_count = config_.period_size;
    int stereo_buffer_bytes = pcm_frames_to_bytes(pcm_in_, stereo_frame_count);
    
    // 1. 临时存储从硬件读来的【双声道】原生数据
    std::vector<int16_t> stereo_buffer(stereo_frame_count * 2); // size * 2 channels
    
    // 2. 准备一个容器存储转换后的【单声道】数据
    std::vector<int16_t> mono_buffer(stereo_frame_count);       // size * 1 channel

    int counter = 0; // 计数器
    while (is_running_.load()) {
        // pcm_read 是阻塞的，读取双声道数据 (L, R, L, R...)
        int ret = pcm_read(pcm_in_, stereo_buffer.data(), stereo_buffer_bytes);
        
        if (ret >= 0) {
            // --- [软件转换：双声道 -> 单声道] ---
            // 我们只需要左声道数据 (Left Channel)，丢弃右声道
            for (int i = 0; i < stereo_frame_count; ++i) {
                mono_buffer[i] = stereo_buffer[2 * i]; // 取偶数位索引
            }
            
            // [新增] 如果开启了文件录制，把单声道数据写入文件
            {
                std::lock_guard<std::mutex> lock(file_mutex_);
                if (record_fp_) {
                    fwrite(mono_buffer.data(), sizeof(int16_t), mono_buffer.size(), record_fp_);
                }
            }

            // 成功读取并转换，放入队列 (Snowboy 需要单声道)
            std::lock_guard<std::mutex> lock(record_mutex_);
            recorded_queue_.push(mono_buffer);
        } else {
            // --- [核心修复] 错误处理与恢复 ---
            
            // 1. 打印具体的错误码 (ret 通常返回 -1, 需要看 errno, 或者 pcm_read 返回的就是负的错误码)
            // TinyALSA 的 pcm_read 出错时通常返回 -1，具体错误在 errno 中；或者直接返回负数
            printf("[Audio] Capture failed! ret: %d, Msg: %s\n", ret, pcm_get_error(pcm_in_));

            // 2. 尝试处理 XRUN (Broken Pipe)
            // 如果是因为缓冲区溢出 (EPIPE)，我们需要重新 prepare 声卡
            if (pcm_is_ready(pcm_in_)) {
                printf("[Audio] Recovering from XRUN...\n");
                pcm_prepare(pcm_in_);
            } else {
                // 如果声卡彻底挂了，尝试重新 open (这是最后的手段)
                printf("[Audio] Sound card not ready, trying to reopen...\n");
                pcm_close(pcm_in_);
                pcm_in_ = pcm_open(0, 0, PCM_IN, &config_);
            }
            
            // 3. 避免死循环刷屏，稍微睡一下
            usleep(20000); 
        }
    }
}

void AudioProcess::PlayLoop() {
    printf("[Audio] Playback Thread Started (Software: 1ch -> Hardware: 2ch).\n");

    // [适配 RV1106] 硬件必须开启双声道
    config_.channels = 2;

    // 打开播放设备 (card 0, device 0)
    pcm_out_ = pcm_open(0, 0, PCM_OUT, &config_);

    if (!pcm_out_ || !pcm_is_ready(pcm_out_)) {
        printf("[Audio] Error opening Playback: %s\n", pcm_get_error(pcm_out_));
        if (pcm_out_) pcm_close(pcm_out_);
        return;
    }

    while (is_running_.load()) {
        std::vector<int16_t> mono_frame;
        
        // 1. 从队列取出【单声道】数据
        {
            std::unique_lock<std::mutex> lock(playback_mutex_);
            // 等待数据或者停止信号
            playback_cv_.wait(lock, [this] {
                return !playback_queue_.empty() || !is_running_.load();
            });

            if (!is_running_.load()) break;

            mono_frame = playback_queue_.front();
            playback_queue_.pop();
        }

        // --- [软件转换：单声道 -> 双声道] ---
        // 为了让双声道硬件正常播放单声道音频，我们需要把数据复制一份
        // Mono: [A, B, C] -> Stereo: [A, A, B, B, C, C]
        std::vector<int16_t> stereo_frame(mono_frame.size() * 2);
        
        for (size_t i = 0; i < mono_frame.size(); ++i) {
            stereo_frame[2 * i]     = mono_frame[i]; // Left
            stereo_frame[2 * i + 1] = mono_frame[i]; // Right
        }

        // 2. 写入硬件播放【双声道数据】
        // 注意：pcm_write 需要的是字节数，这里用 vector 的 size * sizeof(int16_t) 最稳妥
        int ret = pcm_write(pcm_out_, stereo_frame.data(), stereo_frame.size() * sizeof(int16_t));
        
        if (ret < 0) {
             printf("[Audio] Playback write error: %s\n", pcm_get_error(pcm_out_));
        }
    }
}