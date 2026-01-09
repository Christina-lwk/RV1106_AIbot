#ifndef SPEAKING_STATE_H
#define SPEAKING_STATE_H

#include "state_base.h"
#include <string>

class SpeakingState : public StateBase {
    bool has_audio_;
public:
    // 构造函数接收一个 bool，表示是否成功下载了音频
    SpeakingState(bool success) : has_audio_(success) {}
    
    void Enter(ChatContext* ctx) override;
    StateBase* Update(ChatContext* ctx) override;
    void Exit(ChatContext* ctx) override;
    std::string Name() const override { return "Speaking"; }
};

#endif