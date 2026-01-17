#include "chat_app.h"
#include <iostream>
#include <stdlib.h> // for system()
#include "states/idle_state.h"

ChatApp::ChatApp() : current_state_(nullptr) {}

ChatApp::~ChatApp() {
    if (current_state_) {
        current_state_->Exit(&ctx_);
        delete current_state_;
        current_state_ = nullptr;
    }
}

void ChatApp::Init() {
    std::cout << "[ChatApp] Initializing..." << std::endl;

    // 1. 初始化 UI (暂时注释掉，防止报错)
    // if (ctx_.ui) ctx_.ui->Init();

    // 2. 播放开机声音
    // 使用 aplay，这是板子自带的播放命令
    system("aplay assets/greeting.wav");

    // 3. 启动音频服务后台线程:启动 RecordLoop 和 PlayLoop
    if (AudioProcess::GetInstance().Start()) {
        std::cout << "[ChatApp] Audio Service Started Successfully." << std::endl;
    } else {
        std::cerr << "[ChatApp] ERROR: Failed to start Audio Service!" << std::endl;
    }

    // 3. 进入待机状态
    ChangeState(new IdleState());
}

void ChatApp::RunOnce() {
    if (ctx_.should_exit) return;

    if (!current_state_) {
        // 如果这里打印了，说明状态机丢了！可能是 Init 里没 SetState 成功
        printf("[Error] ChatApp::RunOnce: current_state_ is NULL!\n");
        return;
    }

    if (current_state_) {
        StateBase* next_state = current_state_->Update(&ctx_);
        if (next_state != nullptr) {
            ChangeState(next_state);
        }
    }
}

void ChatApp::ChangeState(StateBase* new_state) {
    if (!new_state) return;
    if (current_state_) {
        current_state_->Exit(&ctx_);
        delete current_state_;
    }
    current_state_ = new_state;
    if (current_state_) {
        current_state_->Enter(&ctx_);
    }
}