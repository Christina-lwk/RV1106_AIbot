#ifndef CHAT_STATE_BASE_H
#define CHAT_STATE_BASE_H

#include <string>
#include <memory>
#include "../chat_context.h"

// 状态基类
class StateBase {
public:
    virtual ~StateBase() = default;

    // 当状态机进入该状态时调用 (e.g. 开始录音, 开始播放动画)
    virtual void Enter(ChatContext* ctx) = 0;

    // 状态的主循环，返回“下一个状态的指针”，如果不切换则返回 nullptr
    virtual StateBase* Update(ChatContext* ctx) = 0;

    // 当状态机离开该状态时调用 (e.g. 停止录音, 清理资源)
    virtual void Exit(ChatContext* ctx) = 0;

    // 调试用名称
    virtual std::string Name() const = 0;
};

#endif