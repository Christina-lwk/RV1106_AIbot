#ifndef IDLE_STATE_H
#define IDLE_STATE_H

#include "state_base.h"

class IdleState : public StateBase {
public:
    void Enter(ChatContext* ctx) override;
    StateBase* Update(ChatContext* ctx) override;
    void Exit(ChatContext* ctx) override;
    std::string Name() const override { return "Idle"; }
};

#endif