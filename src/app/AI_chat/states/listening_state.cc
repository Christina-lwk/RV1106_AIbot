#include "states/listening_state.h"
#include "states/thinking_state.h"
#include "services/audio/AudioProcess.h"

#include <iostream>
#include <unistd.h> // for sleep/usleep
#include <stdlib.h> // for system

// 定义文件名
#define RECORD_FILE "user_input.wav"
#define WAKE_REPLY_SOUND "assets/hm.wav"

void ListeningState::Enter(ChatContext* ctx) {
    //回应音效
    AudioProcess::GetInstance().PlayWavFile(WAKE_REPLY_SOUND);
    sleep(1);

    //开始录音
    std::cout << ">>> [State] LISTENING: Start Recording..." << std::endl;
    AudioProcess::GetInstance().SaveStart(RECORD_FILE);
}

StateBase* ListeningState::Update(ChatContext* ctx) {
    // 阻塞5秒，模拟录音过程
    // 因为 AudioProcess 是独立线程，这里的主线程 sleep 不会中断录音
    sleep(5); 

    return new ThinkingState();
}

void ListeningState::Exit(ChatContext* ctx) {
    std::cout << ">>> [State] Exit LISTENING (Processing Audio)" << std::endl;
    //停止写入文件
    AudioProcess::GetInstance().SaveStop();
}