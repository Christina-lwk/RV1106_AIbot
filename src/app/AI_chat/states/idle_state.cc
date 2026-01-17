#include "states/idle_state.h"
#include "chat_context.h" // 确保能访问 ctx->audio
#include "states/listening_state.h"
#include "services/audio/AudioProcess.h"

#include <iostream>
#include <vector>
#include <cmath> // 引入数学库计算音量

// 定义模型路径 (根据你的实际部署路径调整)
#define RESOURCE_FILE "third_party/snowboy/resources/common.res"
#define MODEL_FILE    "third_party/snowboy/resources/snowboy.umdl"

IdleState::IdleState() {
    // 1. 在构造时加载模型，避免每次进入状态都读取磁盘
    try {
        // 创建检测器
        detector_.reset(new snowboy::SnowboyDetect(RESOURCE_FILE, MODEL_FILE));
        
        // 配置参数 (参考 Demo)
        detector_->SetSensitivity("0.5");
        detector_->SetAudioGain(1.0);
        detector_->ApplyFrontend(false);
        
        is_detector_initialized_ = true;
        std::cout << "[IdleState] Snowboy model loaded successfully." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[IdleState] ERROR: Failed to load Snowboy: " << e.what() << std::endl;
        is_detector_initialized_ = false;
    }
}

void IdleState::Enter(ChatContext* ctx) {
    std::cout << ">>> Entering Idle State (Listening for Wake Word...)" << std::endl;
    
    // 确保第一次进入时清空一下旧缓存（防止启动时的杂音误触）
    AudioProcess::GetInstance().ClearBuff();
}

StateBase* IdleState::Update(ChatContext* ctx) {
    // 1. 安全检查
    if (!is_detector_initialized_) {
        return nullptr; 
    }
    // 2. 循环获取音频数据
    std::vector<int16_t> data;
    
    while (AudioProcess::GetInstance().GetFrame(data)) {
        // 3. 喂给 Snowboy 进行检测
        // data.data() 是指针, data.size() 是长度
        int result = detector_->RunDetection(data.data(), data.size());

        // result > 0 表示检测到了唤醒词 (返回的是唤醒词的索引，比如 1)
        if (result > 0) {
            std::cout << "\n>>> WAKE WORD DETECTED! (Index: " << result << ") <<<" << std::endl;
            
            // 4. 切换状态前，清空旧的录音数据，防止把刚才喊唤醒词的声音也录进去
            AudioProcess::GetInstance().ClearBuff();

            // 5. 切换到 Listening 状态
            // 状态机引擎会销毁当前的 IdleState，并 Enter 新的 ListeningState
            return new ListeningState();
        }
    }

    // 没有检测到唤醒，保持当前状态
    return nullptr;
}

void IdleState::Exit(ChatContext* ctx) {
    std::cout << "<<< Exiting Idle State" << std::endl;
    
    // 6. 停止录音，节省资源
    // ctx->audio->StopRecording();
}