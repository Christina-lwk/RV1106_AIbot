#ifndef AUDIO_PROCESS_H
#define AUDIO_PROCESS_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <string>
#include <condition_variable>
#include <cmath> // for RMS

// TinyALSA 头文件
#include <tinyalsa/asoundlib.h>

class AudioProcess {
public:
    // 单例模式
    static AudioProcess& GetInstance() {
        static AudioProcess instance;
        return instance;
    }

    // 禁止拷贝
    AudioProcess(const AudioProcess&) = delete;
    void operator=(const AudioProcess&) = delete;

    bool Init();
    bool Start();
    void Stop();

    // 录音接口
    bool GetFrame(std::vector<int16_t>& chunk);
    void ClearBuff();
    void SaveStart(const std::string& filename);
    void SaveStop();

    // 播放接口
    void PutFrame(const std::vector<int16_t>& pcm_frame);
    void PlayWavFile(const std::string& filename);
    
    // [修改] 查询播放状态 (改为检查队列是否为空)
    bool IsPlaying(); 

    // [新增] 计算 RMS 能量 (静态工具函数)
    static double CalculateRMS(const std::vector<int16_t>& data);

private:
    AudioProcess();
    ~AudioProcess();

    void RecordLoop();
    void PlayLoop();

    // 状态控制
    std::atomic<bool> is_running_{false};

    // 录音相关
    std::thread record_thread_;
    std::mutex record_mutex_;
    std::queue<std::vector<int16_t>> recorded_queue_;
    struct pcm_config config_;
    struct pcm* pcm_in_ = nullptr;
    
    // 文件录制
    std::mutex file_mutex_;
    FILE* record_fp_ = nullptr;

    // 播放相关
    std::thread play_thread_;
    std::mutex playback_mutex_;
    std::condition_variable playback_cv_;
    std::queue<std::vector<int16_t>> playback_queue_;
    struct pcm* pcm_out_ = nullptr;
};

#endif // AUDIO_PROCESS_H