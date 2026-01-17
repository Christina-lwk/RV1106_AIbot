#ifndef IDLE_STATE_H
#define IDLE_STATE_H

#include "state_base.h"
#include "snowboy/snowboy-detect.h"
#include <memory>           // 引用智能指针

class IdleState : public StateBase {
public:
    // 构造函数：我们可以在这里就加载模型，或者在 Init 中加载
    IdleState(); 
    virtual ~IdleState() = default;

    void Enter(ChatContext* ctx) override;
    StateBase* Update(ChatContext* ctx) override;
    void Exit(ChatContext* ctx) override;
    std::string Name() const override { return "Idle"; }

private:
    // 使用智能指针管理 Snowboy 对象，自动释放内存
    std::unique_ptr<snowboy::SnowboyDetect> detector_;
    
    // 标记是否已经初始化成功
    bool is_detector_initialized_ = false;
};

#endif