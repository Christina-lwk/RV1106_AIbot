#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <string>
#include <mutex>

class NetworkClient {
public:
    // 单例模式获取实例
    static NetworkClient& GetInstance() {
        static NetworkClient instance;
        return instance;
    }

    // 禁止拷贝和赋值
    NetworkClient(const NetworkClient&) = delete;
    void operator=(const NetworkClient&) = delete;

    // 初始化 curl
    void init();

    // 设置服务器 IP
    void SetServerIP(const std::string& ip);

    // --- 核心功能 ---

    // ✅ [修复报错] 补上这个函数的声明
    std::string SendRequest(const std::string& endpoint);

    // 上传音频
    std::string SendAudio(const std::string& filepath, bool& out_should_exit);
    // 下载文件
    bool DownloadFile(const std::string& url_path, const std::string& save_path);

private:
    // 私有构造函数
    NetworkClient() {}
    ~NetworkClient() {}

    std::string server_ip_ = "192.168.137.1";
    int port_ = 5000;
};

#endif // NETWORK_CLIENT_H