#include "chat_app.h"
#include <iostream>
#include <unistd.h> 

// 注意：现在入口状态变成了 Listening，而不是 Idle
#include "states/listening_state.h" 

ChatApp::ChatApp() : is_running_(false), current_state_(nullptr) {}

ChatApp::~ChatApp() {
    Stop(); // 析构时确保清理
}

void ChatApp::Init() {
    std::cout << "[ChatApp] Initializing Services..." << std::endl;

    // 1. 启动音频服务后台线程
    // AudioProcess 是单例，Start 可以多次调用(内部有判断)，确保它是运行的
    if (AudioProcess::GetInstance().Start()) {
        std::cout << "✅ [ChatApp] Audio Service Ready." << std::endl;
    } else {
        std::cerr << "❌ [ChatApp] ERROR: Failed to start Audio Service!" << std::endl;
    }

    // 注意：Init 结束后，is_running_ 依然是 false，状态依然是 nullptr
    // 我们在等待 main 函数检测到唤醒词后调用 Start()
}

void ChatApp::Start() {
    if (is_running_) return; // 防止重复启动

    std::cout << ">>> [ChatApp] Starting Session..." << std::endl;
    
    // 1. 重置上下文标志位
    ctx_.should_exit = false;

    // 2. 播放开机/唤醒音效 (使用 AudioProcess，不要用 system)
    // 假设你有一个简短的 'du.wav' 或 'hi.wav'
    // AudioProcess::GetInstance().PlayWavFile("assets/sounds/greeting.wav");

    // 3. 直接进入监听状态 (因为是在 main 里被唤醒的)
    ChangeState(new ListeningState());

    is_running_ = true;
}

void ChatApp::Stop() {
    std::cout << "<<< [ChatApp] Stopping Session..." << std::endl;
    
    // 清理当前状态
    if (current_state_) {
        current_state_->Exit(&ctx_);
        delete current_state_;
        current_state_ = nullptr;
    }

    is_running_ = false;
}

void ChatApp::RunOnce() {
    // 如果没运行，直接返回
    if (!is_running_) return;

    // 1. 检查是否有强制退出标志 (来自 NetworkClient 解析到的 "再见")
    if (ctx_.should_exit) {
        std::cout << "✅ [ChatApp] Exit flag detected. Closing app." << std::endl;
        Stop();
        return;
    }

    // 2. 安全检查
    if (!current_state_) {
        // 如果运行中状态却为空，说明出错了，强制停止
        Stop();
        return;
    }

    // 3. 状态机流转
    StateBase* next_state = current_state_->Update(&ctx_);

    // 如果状态发生了变化
    if (next_state != current_state_) {
        // 这里的逻辑很关键：
        // 如果 next_state 是 nullptr，说明 SpeakingState 觉得聊完了，返回了空
        // ChangeState 会处理 nullptr (删除 current，置空)
        ChangeState(next_state);
        
        // 再次检查，如果切到了空状态，说明 App 该结束了
        if (current_state_ == nullptr) {
            std::cout << "✅ [ChatApp] Conversation finished normally." << std::endl;
            Stop();
        }
    }
}

void ChatApp::ChangeState(StateBase* new_state) {
    // 即使 new_state 是 nullptr，我们也需要执行清理旧状态的逻辑
    if (current_state_) {
        current_state_->Exit(&ctx_);
        delete current_state_;
    }
    
    current_state_ = new_state;
    
    if (current_state_) {
        current_state_->Enter(&ctx_);
    }
}