#ifndef LISTENING_STATE_H
#define LISTENING_STATE_H

#include "states/state_base.h"

class ListeningState : public StateBase {
public:
    ListeningState(); // 构造函数初始化变量

    void Enter(ChatContext* ctx) override;
    StateBase* Update(ChatContext* ctx) override;
    void Exit(ChatContext* ctx) override;
    
    std::string Name() const override { return "Listening"; }

private:
    // VAD 状态变量
    int silence_counter_;      // 连续静音帧数
    int total_frames_;         // 总录制帧数
    bool has_speech_started_;  // 是否检测到过语音
};

#endif