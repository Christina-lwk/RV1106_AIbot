#ifndef LISTENING_STATE_H
#define LISTENING_STATE_H

#include "state_base.h"

class ListeningState : public StateBase {
    int timer_ = 0;
public:
    void Enter(ChatContext* ctx) override;
    StateBase* Update(ChatContext* ctx) override;
    void Exit(ChatContext* ctx) override;
    std::string Name() const override { return "Listening"; }
};

#endif