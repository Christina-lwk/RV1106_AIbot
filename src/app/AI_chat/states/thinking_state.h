#ifndef THINKING_STATE_H
#define THINKING_STATE_H

#include "state_base.h"

class ThinkingState : public StateBase {
public:
    void Enter(ChatContext* ctx) override;
    StateBase* Update(ChatContext* ctx) override;
    void Exit(ChatContext* ctx) override;
    std::string Name() const override { return "Thinking"; }
};

#endif