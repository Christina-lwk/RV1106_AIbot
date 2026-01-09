#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <string>

class NetworkClient {
public:
    static void init(); // 全局初始化 curl

    // 设置服务器 IP
    void SetServerIP(const std::string& ip);

    // 上传音频，返回服务器的 JSON 回复
    std::string SendAudio(const std::string& filepath);

    // 从 URL 下载文件并保存到本地
    bool DownloadFile(const std::string& url_path, const std::string& save_path);

    // 原有的测试函数 (保留以免报错，虽然不用了)
    std::string echoTest(const std::string& msg);

private:
    std::string server_ip_ = "127.0.0.1"; // 默认值
    int port_ = 5000;
};

#endif