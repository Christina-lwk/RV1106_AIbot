#ifndef CHAT_APP_H
#define CHAT_APP_H

#include "chat_context.h"
#include "states/state_base.h"

class ChatApp {
public:
    ChatApp();
    ~ChatApp();
    
    // 初始化硬件和服务 (AudioProcess 等)，但不启动业务逻辑
    void Init();

    // [新增] 启动 App (当 main 检测到唤醒词时调用)
    void Start();

    // [新增] 停止 App (内部调用，或者强制退出时调用)
    void Stop();

    // 状态查询
    bool IsRunning() const { return is_running_; }

    // 核心心跳函数
    void RunOnce();

private:
    bool is_running_ = false; // 标志位

    // 内部切换状态的辅助函数
    void ChangeState(StateBase* new_state);

    // 上下文
    ChatContext ctx_;

    // 当前状态
    StateBase* current_state_;
};

#endif // CHAT_APP_H