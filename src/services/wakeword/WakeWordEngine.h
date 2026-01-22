#ifndef WAKE_WORD_ENGINE_H
#define WAKE_WORD_ENGINE_H

#include <memory>
#include <string>
#include <vector>
#include <cstdint>

// 前置声明，避免在头文件中引入复杂的 snowboy 头文件
namespace snowboy {
    class SnowboyDetect;
}

class WakeWordEngine {
public:
    // 获取单例 (可选，如果你想全局访问)
    // 或者直接作为普通类在 main 中实例化，这里设计为普通类更灵活
    WakeWordEngine();
    ~WakeWordEngine();

    // 初始化模型
    // 返回 true 表示加载成功
    bool Init(const std::string& res_path, const std::string& model_path);

    // 检测一帧音频
    // data: 单声道、16k、16bit 的 PCM 数据
    // 返回值: 
    //   > 0 : 检测到唤醒词 (返回唤醒词索引)
    //   0   : 无事发生
    //   < 0 : 出错
    int Detect(const std::vector<int16_t>& data);
    int Detect(const int16_t* data, int len);

private:
    std::unique_ptr<snowboy::SnowboyDetect> detector_;
    bool is_initialized_ = false;
};

#endif // WAKE_WORD_ENGINE_H