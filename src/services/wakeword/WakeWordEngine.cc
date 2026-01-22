/**
 * @class WakeWordEngine
 * * @note 关于 Detect 函数的重载设计 (Overload Design):
 * 1. 易用性 (User-Friendly): 提供 std::vector 版本，方便上层业务直接调用，符合 C++ 现代风格。
 * 2. 高性能 & 通用性 (Zero-Copy): 提供 (ptr, len) 指针版本，作为核心实现。
 * - 避免了在该函数内部进行不必要的 vector 内存拷贝。
 * - 兼容所有底层数据源 (如 RingBuffer、C 数组、mmap 内存)，不强制依赖 vector。
 * 3. 代码复用 (DRY): vector 版本仅作为“外壳”转发调用，核心逻辑只维护一份。
 */

#include "WakeWordEngine.h"
#include "snowboy/snowboy-detect.h"
#include <iostream>

WakeWordEngine::WakeWordEngine() {}

WakeWordEngine::~WakeWordEngine() {
    // unique_ptr 会自动释放 detector_
}

bool WakeWordEngine::Init(const std::string& res_path, const std::string& model_path) {
    try {
        // 创建检测器
        detector_.reset(new snowboy::SnowboyDetect(res_path, model_path));

        // 配置参数 (保持和你之前的一致)
        detector_->SetSensitivity("0.5");
        detector_->SetAudioGain(1.0);
        detector_->ApplyFrontend(false);

        is_initialized_ = true;
        std::cout << "✅ [WakeWord] Snowboy initialized successfully." << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cout << "❌ [WakeWord] Init Failed: " << e.what() << std::endl;
        is_initialized_ = false;
        return false;
    }
}

int WakeWordEngine::Detect(const std::vector<int16_t>& data) {
    return Detect(data.data(), data.size());
}

int WakeWordEngine::Detect(const int16_t* data, int len) {
    if (!is_initialized_ || !detector_) {
        return -1;
    }
    // RunDetection 是 Snowboy 的核心 API
    return detector_->RunDetection(data, len);
}