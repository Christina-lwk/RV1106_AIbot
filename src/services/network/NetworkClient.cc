#include "services/network/NetworkClient.h"
#include <curl/curl.h>
#include <iostream>
#include <stdio.h>
#include <string>

// 回调：把收到的数据拼接成 string (用于接收 JSON)
static size_t WriteStringCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// 回调：把收到的数据写入文件 (用于下载音频)
static size_t WriteFileCallback(void *ptr, size_t size, size_t nmemb, void *stream) {
    return fwrite(ptr, size, nmemb, (FILE *)stream);
}

// [新增] 简单的 JSON 布尔值解析辅助函数
// 查找 "key": true 或 "key":true
static bool ParseBoolFromJson(const std::string& json, const std::string& key) {
    std::string pattern1 = "\"" + key + "\": true";
    std::string pattern2 = "\"" + key + "\":true";
    
    if (json.find(pattern1) != std::string::npos || 
        json.find(pattern2) != std::string::npos) {
        return true;
    }
    return false;
}

void NetworkClient::init() {
    curl_global_init(CURL_GLOBAL_ALL);
}

void NetworkClient::SetServerIP(const std::string& ip) {
    server_ip_ = ip;
}

std::string NetworkClient::SendRequest(const std::string& endpoint) {
    // 简单的 GET 请求测试 (用于 Ping)
    CURL* curl = curl_easy_init();
    std::string response;
    if(curl) {
        std::string url = "http://" + server_ip_ + ":" + std::to_string(port_) + "/" + endpoint;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteStringCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return response;
}

// [核心实现] 上传音频
std::string NetworkClient::SendAudio(const std::string& filepath, bool& out_should_exit) {
    CURL* curl = curl_easy_init();
    std::string response;
    
    out_should_exit = false;

    // 定义 header 链表指针
    struct curl_slist *headerlist = NULL;

    if (curl) {
        // 拼接 URL: http://IP:5000/chat
        std::string url = "http://" + server_ip_ + ":" + std::to_string(port_) + "/chat";
        
        curl_mime* mime = curl_mime_init(curl);
        curl_mimepart* part = curl_mime_addpart(mime);
        
        // 表单字段名 "audio"
        curl_mime_name(part, "audio");
        // 文件路径
        curl_mime_filedata(part, filepath.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        
        // [核心修改]
        // 1. 添加 "Expect:" 头部（值为空）,禁用复杂传输方式
        headerlist = curl_slist_append(headerlist, "Expect:");
        headerlist = curl_slist_append(headerlist, "Transfer-Encoding:");
        // 2. 将 header 应用到 curl
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteStringCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        // 超时时间
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L); 

        CURLcode res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            std::cerr << "❌ [Network] Upload Error: " << curl_easy_strerror(res) << std::endl;
        } else {
            if (ParseBoolFromJson(response, "should_end_session")) {
                out_should_exit = true;
                std::cout << "✅ [Network] Exit signal received from Server." << std::endl;
            }
        }

        // --- [清理] 别忘了释放 headerlist ---
        curl_slist_free_all(headerlist);
        
        curl_mime_free(mime);
        curl_easy_cleanup(curl);
    }
    return response;
}

// [核心实现] 下载文件
bool NetworkClient::DownloadFile(const std::string& url_path, const std::string& save_path) {
    CURL* curl = curl_easy_init();
    bool success = false;

    if (curl) {
        FILE* fp = fopen(save_path.c_str(), "wb");
        if (fp) {
            std::string full_url = "http://" + server_ip_ + ":" + std::to_string(port_) + url_path;
            
            curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFileCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            
            // ✅ [Fix] 下载也增加超时时间，防止生成回复太慢
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

            CURLcode res = curl_easy_perform(curl);
            if(res == CURLE_OK) {
                success = true;
            } else {
                std::cerr << "❌ [Network] Download Error: " << curl_easy_strerror(res) << std::endl;
            }

            fclose(fp);
        } else {
            std::cerr << "❌ [Network] Cannot open file for writing: " << save_path << std::endl;
        }
        curl_easy_cleanup(curl);
    }
    return success;
}