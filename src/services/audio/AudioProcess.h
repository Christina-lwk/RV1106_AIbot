#ifndef AUDIOPROCESS_H
#define AUDIOPROCESS_H

#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <cstdint>

// 替换 PortAudio 为 Tinyalsa
extern "C" {
    #include <tinyalsa/asoundlib.h>
}

class AudioProcess {
public:
    // 构造函数
    AudioProcess(int sample_rate = 16000, int channels = 1, int frame_duration_ms = 40);
    ~AudioProcess();

    // --- 核心控制接口 (保持不变) ---
    bool start(); // 同时启动录音和播放 (简化用于测试)
    void stop();

    // --- 数据接口 ---
    // 从麦克风获取数据 (供 WebSocket 发送用)
    bool getRecordedAudio(std::vector<int16_t>& recordedData);
    
    // 往喇叭塞数据 (供 WebSocket 接收用)
    void addFrameToPlaybackQueue(const std::vector<int16_t>& pcm_frame);
    
    // 读取并播放 wav 文件
    void playWavFile(const char* filename);

private:
    // 音频参数
    int sample_rate;
    int channels;
    int frame_duration_ms;
    unsigned int period_size;

    // 线程控制
    std::atomic<bool> isRunning;
    std::thread recordThread;
    std::thread playThread;

    // Tinyalsa 设备句柄
    struct pcm* pcm_in;  // 录音设备
    struct pcm* pcm_out; // 播放设备

    // 队列与同步
    std::queue<std::vector<int16_t>> recordedQueue;
    std::mutex recordMutex;
    std::condition_variable recordCV;

    std::queue<std::vector<int16_t>> playbackQueue;
    std::mutex playbackMutex;
    std::condition_variable playbackCV;

    // 内部线程函数
    void recordLoop();
    void playLoop();
};

#endif // AUDIOPROCESS_H