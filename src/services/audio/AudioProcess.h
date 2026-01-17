#ifndef AUDIO_PROCESS_H
#define AUDIO_PROCESS_H

#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <string>
#include <cstdint>
#include <cstdio>

#include <tinyalsa/pcm.h> 

class AudioProcess {
public:
    // 1. 单例访问 (Meyers Singleton)
    static AudioProcess& GetInstance() {
        static AudioProcess instance;
        return instance;
    }

    // 禁止拷贝和赋值
    AudioProcess(const AudioProcess&) = delete;
    void operator=(const AudioProcess&) = delete;

    // --- [功能 1] 系统控制 ---
    // 初始化音频配置 (不开启线程)
    bool Init();
    // 启动录音和播放线程
    bool Start();
    // 停止线程并释放资源
    void Stop();
    
    // --- [功能 2] 录音数据接口 (供 IdleState/Snowboy 使用) ---
    // [关键] 非阻塞获取一段音频。如果队列有数据，填入 chunk 并返回 true；否则返回 false。
    bool GetFrame(std::vector<int16_t>& chunk);
    
    // [关键] 清空录音队列 (状态切换时调用，防止处理旧声音)
    void ClearBuff();

    // --- [功能 3] 播放接口 (供 SpeakingState 使用) ---
    // 添加 PCM 数据到播放队列
    void PutFrame(const std::vector<int16_t>& pcm_frame);
    // 直接播放 WAV 文件 (封装了解析 WAV 头的逻辑)
    void PlayWavFile(const std::string& filename);

    // --- [功能 4] 录制用户语音到文件---
    // start: 创建文件并预留 WAV 头位置
    void SaveStart(const std::string& filename);
    // stop: 回填 WAV 头并关闭文件
    void SaveStop();

private:
    AudioProcess();
    ~AudioProcess();

    // 线程工作函数
    void RecordLoop();
    void PlayLoop();

    // 硬件参数配置 (使用 tinyalsa 原生结构体)
    struct pcm_config config_;

    // 线程控制
    std::atomic<bool> is_running_{false};
    std::thread record_thread_;
    std::thread play_thread_;

    // TinyALSA 句柄
    struct pcm* pcm_in_ = nullptr;  // 录音句柄
    struct pcm* pcm_out_ = nullptr; // 播放句柄

    // --- 数据队列 ---
    
    // 录音队列 (麦克风 -> Snowboy)
    // 只需要互斥锁，不需要条件变量，因为消费者是非阻塞轮询
    std::queue<std::vector<int16_t>> recorded_queue_;
    std::mutex record_mutex_;

    // 播放队列 (TTS -> 扬声器)
    // 需要条件变量，因为播放线程在没声音时应该休眠省电
    std::queue<std::vector<int16_t>> playback_queue_;
    std::mutex playback_mutex_;
    std::condition_variable playback_cv_;
    
    // 写入 WAV 头的辅助函数
    void WriteWavHeader(FILE* fp, int data_size);

    // 录音文件句柄
    FILE* record_fp_ = nullptr;
    std::mutex file_mutex_; // 保护文件操作
};

#endif // AUDIO_PROCESS_H