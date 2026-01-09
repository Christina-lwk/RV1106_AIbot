#include "thinking_state.h"
#include <iostream>
#include <string>
#include "speaking_state.h" 

void ThinkingState::Enter(ChatContext* ctx) {
    std::cout << ">>> [State]THINKING: Uploading" << std::endl;
    // 如果有UI，这里调用 ctx->ui->ShowThinking();
}

StateBase* ThinkingState::Update(ChatContext* ctx) {
    // 1. 设置服务器 IP
    ctx->network->SetServerIP("192.168.137.1"); 

    // 2. 发送刚才录制的真实音频
    // 这个文件是刚才 ListeningState 用 arecord 生成的
    std::cout << "   (Uploading user_input.wav)..." << std::endl;
    std::string json = ctx->network->SendAudio("user_input.wav");
    
    // 3. 检查有没有收到回复
    if (json.empty()) {
        std::cerr << "   (Error: Server No Response)" << std::endl;
        return new SpeakingState(false); // 失败去 Speaking 报个错
    }

    std::cout << "   (Server Reply JSON): " << json << std::endl;

    // 4. 解析 JSON 提取 audio_url
    // 简单查找 "audio_url" 字段
    std::string key = "audio_url";
    size_t found = json.find(key);
    
    if (found != std::string::npos) {
        // 我们知道 Server 总是返回 /get_audio/reply.wav
        // 所以这里可以直接写死路径，或者你可以写复杂的 JSON 解析逻辑
        std::string url = "/get_audio/reply.wav"; 
        
        // 5. 下载回复音频，保存为 reply.wav
        std::cout << "   (Downloading reply)..." << std::endl;
        bool dl_ok = ctx->network->DownloadFile(url, "reply.wav");
        
        if (dl_ok) {
            std::cout << "   (Download Success!)" << std::endl;
            return new SpeakingState(true); // 成功，带参数 true，去播放
        }
    }

    return new SpeakingState(false);
}

void ThinkingState::Exit(ChatContext* ctx) {
    std::cout << ">>> [State] Exit THINKING" << std::endl;
}