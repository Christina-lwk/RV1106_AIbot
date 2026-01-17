#include "states/speaking_state.h"
#include "states/idle_state.h"
#include "services/audio/AudioProcess.h" // 必须引入
#include <iostream>
#include <unistd.h> // for sleep

void SpeakingState::Enter(ChatContext* ctx) {
    std::cout << ">>> [State]SPEAKING" << std::endl;
    
    // [核心修复] 不要用 system("aplay ...")，因为设备被占用了
    // 使用 AudioProcess 内部接口播放
    AudioProcess::GetInstance().PlayWavFile("reply.wav");
}

StateBase* SpeakingState::Update(ChatContext* ctx) {
    // [重要] 我们需要给一点时间让它把话说完
    // 简单做法：这里先死等 5-6 秒 (假设回复不会太长)
    // 进阶做法：可以去读取 wav 文件头计算时长，但现在先由你预估
    
    sleep(6); // 假设回复不超过 6 秒
    
    // 播放结束，回到空闲状态，准备下一次唤醒
    return new IdleState();
}

void SpeakingState::Exit(ChatContext* ctx) {
    std::cout << ">>> [State] Exit SPEAKING" << std::endl;
}