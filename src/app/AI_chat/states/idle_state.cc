#include "idle_state.h"
#include <iostream>
#include "listening_state.h"

void IdleState::Enter(ChatContext* ctx) {
    std::cout << ">>> [State] IDLE" << std::endl;
}

StateBase* IdleState::Update(ChatContext* ctx) {
    // 简单的自动触发逻辑 (每 2 秒触发一次)
    static int counter = 0;
    counter++;
    
    if (counter > 400) { 
        counter = 0;
        std::cout << "!!! WAKE UP !!!" << std::endl;
        return new ListeningState();
    }
    return nullptr;
}

void IdleState::Exit(ChatContext* ctx) {
    std::cout << ">>> [State] Exit IDLE" << std::endl;
}