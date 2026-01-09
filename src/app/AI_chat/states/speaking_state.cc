#include "speaking_state.h"
#include "idle_state.h"
#include <iostream>
#include <stdlib.h> // for system()
#include <unistd.h> // for sleep()

void SpeakingState::Enter(ChatContext* ctx) {
    std::cout << ">>> [State]SPEAKING" << std::endl;
    // 如果你有UI，这里可以 ctx->ui->ShowSpeaking();
}

StateBase* SpeakingState::Update(ChatContext* ctx) {
    if (has_audio_) {
        std::cout << "   (Playing reply.wav)" << std::endl;
        
        // [关键] 调用板子自带的播放器播放下载好的音频
        // system() 是阻塞的，意味着播放完才会往下走，非常适合这里
        int ret = system("aplay reply.wav");
        
        if (ret != 0) {
             std::cout << "   (Error: Playback failed)" << std::endl;
        }
    } else {
        std::cout << "   (Error: No audio to play)" << std::endl;
        // 如果没音频，稍微停顿一下让人看清错误
        sleep(1);
    }

    // 播放完毕，回到 Idle，等待下一次唤醒
    return new IdleState();
}

void SpeakingState::Exit(ChatContext* ctx) {
    std::cout << ">>> [State] Exit SPEAKING" << std::endl;
}