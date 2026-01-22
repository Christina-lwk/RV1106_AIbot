#include "states/speaking_state.h"
#include "states/listening_state.h"
#include "services/audio/AudioProcess.h"
#include <iostream>
#include <unistd.h> // for sleep

void SpeakingState::Enter(ChatContext* ctx) {
    std::cout << ">>> [State]SPEAKING" << std::endl;
    // 使用 AudioProcess 内部接口播放
    AudioProcess::GetInstance().PlayWavFile("reply.wav");
}

StateBase* SpeakingState::Update(ChatContext* ctx) {
    while (AudioProcess::GetInstance().IsPlaying()) {
            usleep(100000); // 100ms
        }

    //检查是否需要退出 App
    if (ctx->should_exit) {
        std::cout << "✅ [State] 'Goodbye' detected. Ending session." << std::endl;
        // 返回 nullptr 表示状态机结束，ChatApp::RunOnce 会捕获到并调用 Stop()
        return nullptr; 
    }
    std::cout << ">>> Loop back to Listening..." << std::endl;
    return new ListeningState();
}

void SpeakingState::Exit(ChatContext* ctx) {
    std::cout << ">>> [State] Exit SPEAKING" << std::endl;
}