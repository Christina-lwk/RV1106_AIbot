#include "states/listening_state.h"
#include "states/thinking_state.h"
#include "services/audio/AudioProcess.h"

#include <iostream>
#include <unistd.h> // for sleep/usleep
#include <stdlib.h> // for system

// 定义文件名
#define RECORD_FILE "user_input.wav"
#define WAKE_REPLY_SOUND "assets/hm.wav"

// VAD 阈值 (需要根据实际麦克风调整，通常 1000-3000)
#define VAD_THRESHOLD 2000 
// 静音判定 (30帧 * 64ms ≈ 2秒)
#define MAX_SILENCE_FRAMES 30
// 最大录音时长 (150帧 * 64ms ≈ 10秒)
#define MAX_RECORD_FRAMES 150

// 构造函数：初始化状态变量
ListeningState::ListeningState() 
    : silence_counter_(0), total_frames_(0), has_speech_started_(false) {}

void ListeningState::Enter(ChatContext* ctx) {
    //hm？
    AudioProcess::GetInstance().PlayWavFile(WAKE_REPLY_SOUND);
    // 使用新的 IsPlaying 接口轮询，看hm音效播放完没
    while (AudioProcess::GetInstance().IsPlaying()) {
        usleep(10000); 
    }

    //开始录音
    std::cout << ">>> [State] LISTENING: Start Recording..." << std::endl;
    AudioProcess::GetInstance().ClearBuff();
    AudioProcess::GetInstance().SaveStart(RECORD_FILE);
}

StateBase* ListeningState::Update(ChatContext* ctx) {
    std::vector<int16_t> frame_data;

    // 尝试获取一帧音频
    if (AudioProcess::GetInstance().GetFrame(frame_data)) {
        double rms = AudioProcess::CalculateRMS(frame_data);
        // 调试 VAD 阈值时可以解开这行
        // printf("RMS: %.0f\n", rms);
        if (rms > VAD_THRESHOLD) {
            // 检测到说话
            if (!has_speech_started_) {
                std::cout << "   (Speech Started...)" << std::endl;
            }
            has_speech_started_ = true;
            silence_counter_ = 0; // 重置静音计数
        } else {
            // 静音
            if (has_speech_started_) {
                silence_counter_++;
            }
        }

        total_frames_++;

        // 判定退出条件
        // 条件 A: 说完话了 (开始过说话 + 连续静音2秒)
        if (has_speech_started_ && silence_counter_ > MAX_SILENCE_FRAMES) {
            std::cout << "✅ [VAD] Speech End Detected." << std::endl;
            return new ThinkingState();
        }

        // 条件 B: 超时 (录太久了)
        if (total_frames_ > MAX_RECORD_FRAMES) {
            std::cout << "[VAD] Timeout." << std::endl;
            return new ThinkingState();
        }

        // 条件 C: 一直没说话 (5秒全是静音)
        if (!has_speech_started_ && total_frames_ > 80) { // 约 5秒
             std::cout << "[VAD] No speech detected." << std::endl;
             // 可以选择重试，或者直接去 Thinking 让 Server 处理空录音
             return new ThinkingState();
        }
    }

    // 继续录音
    return this;
}

void ListeningState::Exit(ChatContext* ctx) {
    std::cout << ">>> [State] Exit LISTENING (Processing Audio)" << std::endl;
    //停止写入文件
    AudioProcess::GetInstance().SaveStop();
}