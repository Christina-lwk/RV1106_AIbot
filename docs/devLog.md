# Echo-Mate 开发复盘：移植snowboy+系统优化

**日期:** 2026-01-10 至 2026-01-17 夜
**对应版本:** Voice-Loop-Alpha-v1
**硬件平台:** Rockchip RV1106

---

## 一、本阶段完成的新任务 (New Achievements)

在本阶段，我们成功打通了从 设备端唤醒 到 云端语义理解 再到 设备端语音回复 的完整闭环。

### 1. 音频底层架构重构
* **软件声通分离 (Soft-Split):** 适配 RV1106 硬件强制双声道 (Stereo) 的特性，在 AudioProcess 内部实现了数据流拆分，提取单声道 (Mono) 数据供 Snowboy 和 Whisper 使用。
* **原生 WAV 封装 (Native WAV):** 移除了板端对 ffmpeg 工具的依赖。在 C++ 层实现了标准 WAV 文件头 (44 bytes Header) 的写入逻辑 (WriteWavHeader)，实现了轻量级、零依赖的录音文件生成。
* **统一音频管辖:** 废弃了所有 system("arecord/aplay") 调用，将麦克风读取和喇叭播放统一收归 AudioProcess 单例管理，彻底解决了多进程争抢声卡的问题。

### 2. 网络通信协议优化
* **HTTP 客户端调优:** 基于 libcurl 实现了稳定的音频文件上传 (Multipart) 和回复下载。
* **协议兼容性加固:** 针对简易 Python 服务器（Flask/Werkzeug）的特性，对 HTTP Header 进行了深度定制，确保传输稳定性。

### 3. 全双工交互逻辑闭环
* **状态机完善:** 实现了 Idle (唤醒) -> Listening (录音 5s) -> Thinking (上传/处理) -> Speaking (播放回复) -> Idle 的自动流转。
* **交互体验提升:** 增加了唤醒提示音 ("Hm/Hi")，并解决了录音与提示音的时序冲突问题。

### 4. 服务端流水线 (Python)
* **全栈处理链:** 搭建了 ASR (Whisper) -> Logic (回声测试) -> TTS (Edge-TTS) -> Format (FFmpeg转码) 的处理流水线。
* **自动容错:** 实现了对 TTS 生成过程中的文件清理和格式强制转换 (MP3 -> WAV 16k)。

---

## 二、踩坑与解决方案 (Pitfalls & Solutions)

这是本阶段最宝贵的经验总结，记录了导致系统崩溃或功能失效的关键问题。

### 1. "哑巴麦克风" 与 资源死锁
* **现象:** 唤醒后无法录音，或者播放时报错 Device or resource busy。
* **原因:** 代码中混合使用了 TinyALSA (代码内) 和 arecord/aplay (Shell 命令)。后者试图打开已被前者独占的 PCM 设备，导致 crash。
* **解决:** 全权接管。删除所有 system() 音频调用，所有录音/播放操作必须通过 AudioProcess::GetInstance() 接口调用。

### 2. "无限刷屏" 的 Broken Pipe (XRUN)
* **现象:** 终端疯狂打印 Capture read error，且 ret 返回值异常。
* **原因:** 1. 硬件生产数据速度 > 软件读取速度（例如双声道数据量翻倍但缓冲区未调整），导致环形缓冲区溢出 (Overrun)。2. RV1106 的 TinyALSA 实现与标准不同，成功时返回帧数 (如 1024) 而非 0。
* **解决:** 1. 修改判断逻辑：if (ret >= 0) 即视为成功。2. 增加恢复机制：检测到 XRUN (EPIPE/-32) 时，自动执行 pcm_prepare() 重置声卡状态。

### 3. Flask 服务器的 "Invalid chunk header"
* **现象:** 板子上传音频时，Python 服务器直接报 500 错误，提示 Chunk 解析失败。
* **原因:** libcurl 默认开启 Expect: 100-continue 和 Transfer-Encoding: chunked。Flask 的开发服务器（Werkzeug）对这种分块传输支持极差，导致协议解析崩溃。
* **解决:** 简单粗暴模式。在 C++ 端显式禁用这两个 Header，强制使用定长 (Content-Length) 传输。

### 4. "消失的 WAV 文件" (依赖缺失)
* **现象:** 服务器报 400 Bad Request (No audio)，因为上传了空文件。
* **原因:** 之前的代码依赖板子执行 system("ffmpeg ...") 来转码。但板子固件精简，根本没有安装 ffmpeg，导致命令静默失败。
* **解决:** C++ 手撸文件头。直接在内存中操作，利用 fseek 预留 44 字节头部，录音结束后回填 WAV Header，不再依赖任何外部工具。

### 5. Python 作用域陷阱
* **现象:** 服务端报错 name 'raw_path' is not defined。
* **原因:** 在 generate_tts_wav 函数中直接使用了 chat 函数里的局部变量 raw_path。
* **解决:** 修正变量传递逻辑，确保每个函数只操作传入的参数。

---

## 三、下一步计划 (Next Steps)

1.  **接入真 AI:** 在 server.py 中接入 DeepSeek 或 ChatGPT API，替换当前的回声逻辑。
2.  **连续对话模式:** 修改状态机转换逻辑，实现“多轮对话”。即只需喊一次 "Snowboy" 唤醒，后续交互无需重复唤醒，直到超时或主动结束。
3.  **VAD 优化:** 引入语音活动检测，实现“说完即停”，不再死板地录音 5 秒。