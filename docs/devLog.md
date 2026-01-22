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

---

## 三、下一步计划 (Next Steps)

1.  **接入真 AI:** 在 server.py 中接入 DeepSeek API，替换当前的回声逻辑。
2.  **连续对话模式:** 修改状态机转换逻辑，实现“多轮对话”。即只需喊一次 "Snowboy" 唤醒，后续交互无需重复唤醒，直到超时或主动结束。
3.  **VAD 优化:** 引入语音活动检测，实现“说完即停”，不再死板地录音 5 秒。

---
<br>

# Echo-Mate 开发复盘：系统重构与真AI接入

**日期:** 2026-01-17 夜 至 2026-01-20
**硬件平台:** Rockchip RV1106

---

## 一、里程碑成就 (Milestones)

在本阶段，我们不仅打通了语音闭环，更完成了从“单片机固件思维”到“嵌入式操作系统思维”的架构跃迁，并赋予了设备真正的智能。

### 1. 架构重构：桌面(Launcher)与应用(App)分离
* **分层设计:** 将原本的单体循环拆解为 **System Layer** (main.cc) 和 **App Layer** (ChatApp)。
    * **System Layer:** 类似于操作系统桌面，负责全局 UI 刷新和 **全局唤醒词监听**。
    * **App Layer:** 类似于一个 APP，仅在唤醒后动态创建，负责录音、网络交互和对话流转。
* **服务解耦:** 将 Snowboy 逻辑从 `IdleState` 彻底剥离，封装为独立的 `WakeWordEngine` 服务 (`src/services/wakeword`)。实现了“平时待机只跑唤醒（省资源），唤醒后才跑业务”的高效模式。

### 2. AI 大脑接入 (Server V2)
* **LLM 集成:** 在服务端 (`server.py`) 接入 **DeepSeek API**，替换了原本的回声测试逻辑。通过 System Prompt 约束 AI 进行简短（50字内）、口语化的回复。
* **短期记忆 (Memory):** 在服务端实现了基于 List 的滑动窗口记忆（Context），现在 AI 能够记住前几轮的对话内容。
* **意图检测 (Intent Detection):** 服务端分析用户文本，识别“再见/退下”等关键词，并通过 JSON 字段 `should_end_session` 指挥设备端退出 App，返回 System Layer。

### 3. 交互体验质变 (VAD & Async)
* **VAD (语音活动检测):** 告别了死板的“定长录音 5s”。在 `ListeningState` 中实现了基于 **RMS (均方根) 能量** 的检测算法，实现了“说话即录，闭嘴即停，超时自动断”的自然交互。
* **精准播放控制:** 移除了 `sleep(6)` 这种不可靠的延时。在 `AudioProcess` 中实现了 `IsPlaying()` 接口（基于播放队列判空），状态机现在能精准知道 AI 何时说完话，实现了无缝的“说完即听”。

---

## 二、踩坑与解决方案 (The Trenches)

这是本阶段最宝贵的经验总结，记录了导致系统崩溃、编译失败或逻辑死锁的关键问题。

### 1. 状态机的 "自杀式退出"
* **现象:** 唤醒后，终端显示 "Recording Started"，但瞬间显示 "Exit LISTENING"，录音文件仅 44 字节（空头），App 直接结束。
* **原因:** 在 `ListeningState::Update` 中，原本意图是“保持当前状态继续录音”，却错误地返回了 `nullptr`。在我们的架构约定中，返回 `nullptr` 意味着结束 App 生命周期。
* **解决:** 将返回值修改为 `return this;`，明确告知状态机引擎保持当前状态。

### 2. "幽灵" 计数器 (Static 变量陷阱)
* **现象:** 第一轮对话正常 VAD 切断，但第二轮对话刚开始就瞬间触发 "Timeout"，导致无法录音。
* **原因:** 在 VAD 逻辑中使用了 `static int counter = 0;`。静态局部变量的生命周期是全局的，App 重启（`new ListeningState`）后，计数器依然保留着上一轮的数值（例如 100），导致直接满足退出条件。
* **解决:** 严禁在状态机中使用 `static` 变量存储状态。将 `silence_counter_` 等变量移至类成员变量，并在构造函数中初始化。

### 3. `memset` 误伤与头文件迷宫
* **现象:** 编译警告提示 `memset` 清空非平凡类型对象；编译报错 `fatal error: include/snowboy-detect.h not found`。
* **原因:** 1. `WavHeader` 包含默认初始化值，`memset` 会破坏结构体特性。
    2. Makefile 里的 `-I` 路径指向了 `third_party/snowboy/include`，代码里却又写了 `#include "include/..."`，导致路径重复（`include/include/...`）。
* **解决:** 1. 移除 `memset`，利用 C++ 构造函数自动初始化 WAV 头。
    2. 修正 Include 路径，直接引用文件名 `#include "snowboy-detect.h"`。

### 4. 原子变量的 "竞态条件"
* **现象:** 播放时偶尔状态机提前切回录音，或者编译器报错 `is_playing_ undefined`。
* **原因:** 试图用一个 `atomic<bool> is_playing_` 手动标记播放状态。但在生产者（读文件）和消费者（播放线程）的间隙，这个变量可能被误置为 false，导致状态机误判。
* **解决:** 废弃手动的原子标志位。改为直接查询 `playback_queue_.empty()`，只要队列里有数据，就认为“正在播放”，逻辑更健壮。

### 5. 构造函数缺失 (Linker Error)
* **现象:** 编译报错 `undefined reference to ListeningState::ListeningState()`。
* **原因:** 在头文件中声明了构造函数用于初始化 VAD 变量，但在 `.cc` 文件中忘记实现它。
* **解决:** 在 `listening_state.cc` 中补全构造函数实现。

---

## 三、系统架构图 (Current Architecture)

```mermaid
graph TD
    subgraph "System Layer (main.cc)"
        Launcher((System Loop))
        WakeEngine[WakeWordEngine\n(Snowboy)]
        Launcher -->|Poll| WakeEngine
    end

    subgraph "App Layer (ChatApp)"
        AppStart((App::Start))
        Listen[ListeningState\n(VAD RMS Check)]
        Think[ThinkingState\n(Network Upload)]
        Speak[SpeakingState\n(Playback Check)]
        
        AppStart --> Listen
        Listen -->|Voice End| Think
        Think -->|Get Reply| Speak
        Speak -->|should_exit=false| Listen
        Speak -->|should_exit=true| AppExit((App::Stop))
    end
    
    subgraph "Services (Singletons)"
        Audio[AudioProcess\n(TinyALSA Thread)]
        Net[NetworkClient\n(Libcurl)]
    end

    WakeEngine -->|Detected!| AppStart
    AppExit -->|Return Control| Launcher
    
    Listen <--> Audio
    Speak <--> Audio
    Think <--> Net