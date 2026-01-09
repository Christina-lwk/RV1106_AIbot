#ifndef CHAT_APP_H
#define CHAT_APP_H

#include "chat_context.h"
#include "states/state_base.h"

class ChatApp {
public:
    ChatApp();
    ~ChatApp();

    // 初始化硬件和服务
    void Init();

    // 核心心跳函数：被 main 主循环调用，每次执行一步逻辑即返回
    void RunOnce();

private:
    // 内部切换状态的辅助函数
    void ChangeState(StateBase* new_state);

    // 机器人“大脑”的所有记忆和感知 (Audio, Network, Flags)
    ChatContext ctx_;

    // 当前所处的状态 (Idle, Listening, Speaking...)
    StateBase* current_state_;
};

#endif // CHAT_APP_H