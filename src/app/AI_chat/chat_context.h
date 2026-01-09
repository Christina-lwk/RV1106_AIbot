#ifndef CHAT_CONTEXT_H
#define CHAT_CONTEXT_H

#include <memory>
#include <string>

// 引入你的服务 (根据你的实际路径调整)
#include "../../services/audio/AudioProcess.h"
#include "../../services/network/NetworkClient.h"

// 前置声明，防止循环引用
class ChatApp;

struct ChatContext {
    // 硬件服务的指针 (使用智能指针管理生命周期)
    std::shared_ptr<AudioProcess> audio;
    std::shared_ptr<NetworkClient> network;
    
    // 全局标志位
    bool should_exit = false;     // 是否退出聊天App返回主页
    std::string last_user_text;   // 刚刚识别到的用户语音文字
    std::string last_ai_reply;    // AI 返回的回复文字

    // 构造函数初始化
    ChatContext() {
        audio = std::make_shared<AudioProcess>();
        network = std::make_shared<NetworkClient>();
    }
};

#endif