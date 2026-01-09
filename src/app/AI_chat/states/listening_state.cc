#include "listening_state.h"
#include <iostream>
#include <stdlib.h> // 必须引入这个，用于调用 system()
#include "thinking_state.h"

void ListeningState::Enter(ChatContext* ctx) {
    std::cout << ">>> [State] LISTENING: Start Recording (5s)" << std::endl;
    // 如果你有屏幕UI，这里可以调用 ctx->ui->ShowListening();
}

StateBase* ListeningState::Update(ChatContext* ctx) {
    // 1. 调用 arecord 真正开始录音
    // system 是阻塞的，程序会在这里停5秒，直到录音结束
    int ret = system("arecord -D hw:0,0 -f S16_LE -r 16000 -c 2 -d 5 user_input.wav");

    if (ret == 0) {
        std::cout << ">>> [Audio] Recording saved to user_input.wav" << std::endl;
    } else {
        // 如果录音失败，打印错误，但我们还是让它去 thinking 状态，
        // 这样至少程序不会卡死，只是上传空文件或旧文件而已。
        std::cerr << ">>> [Audio] Error: Recording failed! (Check mic connection)" << std::endl;
    }

    // 2. 录音结束，立即跳转到“思考”状态去上传
    return new ThinkingState();
}

void ListeningState::Exit(ChatContext* ctx) {
    std::cout << ">>> [State] Exit LISTENING" << std::endl;
}