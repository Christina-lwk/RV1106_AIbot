# Echo‑Mate 技术白皮书（2026.1.17）

## 概要
Echo‑Mate 是运行在 Rockchip RV1106 开发板上的嵌入式 AI 语音助手。客户端为 C++（状态机 + 单例服务），服务端为 Python (Flask)（Whisper + Edge‑TTS）。文档重点说明音频子系统、网络协议兼容性与端侧/服端协同优化。

---

## 1. 系统架构（System Architecture）
- 架构风格：状态机（State Machine）驱动的业务流 + 单例服务（Singleton Services）。
  - 状态机基类与实现：`StateBase`, 状态实现示例：`IdleState` / `ListeningState` / `ThinkingState` / `SpeakingState`。
  - 状态机入口与驱动：`ChatApp` / chat_app.cc。
  - 上下文共享：`ChatContext`。

- 核心单例服务：
  - 音频：`AudioProcess` / AudioProcess.cc（录音/播放线程、队列、WAV 管理）。
  - 网络：`NetworkClient` / NetworkClient.cc（libcurl 封装：上传/下载/超时/头部定制）。
  - 唤醒：基于 Snowboy 的检测在 `IdleState` 中使用（引用 `snowboy-detect`）。

---

## 2. 音频子系统（Audio Subsystem）— 重点更新
1. 采集策略
   - 硬件层使用 TinyALSA 以双声道采集（Stereo, 16 kHz），参考 TinyALSA 接口：[`pcm_read` / `pcm_readi`](include/tinyalsa/pcm.h) 与示例工具 tinycap.c。
   - 软件层在单例服务中做通道分离：`AudioProcess::RecordLoop` 从双声道数据中提取左声道为单声道用于唤醒/录音（Stereo -> Mono），并将单声道数据推入录音队列（供 Snowboy 使用）。

2. WAV 封装（零依赖实现）
   - 废弃板端 FFmpeg 依赖：采用原生 C/C++ 文件操作（`fseek` + `fwrite`）在端侧写入/回填 WAV 头，参考实现：`AudioProcess::SaveStart` 与 `AudioProcess::SaveStop`。
   - 优点：减小运行时依赖与闪存占用，避免跨进程调用带来的资源竞争。

3. 音频设备冲突管理（Device/resource busy）
   - 彻底废弃使用 `system("arecord")` / `system("aplay")` 的做法（相关旧处：chat_app.cc 中开机播放、历史实现），改为由 `AudioProcess` 独占管理所有音频流。
   - Speaking 状态播放改为调用：`AudioProcess::PlayWavFile`（见 speaking_state.cc 的改动），避免 "Device or resource busy" 错误。

4. XRUN / Buffer Overrun 恢复
   - 在采集循环中对错误进行详细日志并实现恢复逻辑：当检测到 XRUN/缓冲溢出，优先尝试 `pcm_prepare` 进行恢复；若设备不可用则尝试重开（见 `AudioProcess::RecordLoop` 的错误处理与重开逻辑）。
   - 该策略降低了在资源受限环境下的录音崩溃概率并提升稳定性。

5. 播放管线与单声道-双声道兼容
   - 播放线程将单声道 PCM 数据复制成双声道（Mono -> Stereo）以适配板端双声道硬件：`AudioProcess::PlayLoop`。

---

## 3. 网络通信协议（Network Protocol）— 重点更新
- 客户端使用 libcurl（已内置头文件：`include/curl/curl.h` / 库头在 curl）做 HTTP 通信，核心实现：`NetworkClient::SendAudio`、`NetworkClient::DownloadFile`、`NetworkClient::SendRequest`。

- 关键兼容性配置
  - 明确禁用 `Expect: 100-continue` 与 `Transfer-Encoding: chunked`，确保 Flask 开发服务器能立即处理表单上传并兼容 Content-Length 传输（实现见：`NetworkClient::SendAudio`，通过 `curl_slist_append(headerlist, "Expect:")` 与 `curl_slist_append(headerlist, "Transfer-Encoding:")`）。
  - 加入请求超时（`CURLOPT_TIMEOUT`）以避免长时间阻塞（见 `NetworkClient::SendAudio` / `NetworkClient::DownloadFile`）。
  - 上传使用 MIME multipart（`curl_mime`），下载使用写文件回调：`WriteFileCallback`。

- 设计目的：在开发阶段与 Flask 简单 HTTP 服务器配合时，减少分块传输导致的兼容性问题与阻塞行为，提升端侧在不稳定网络下的可恢复性。

---

## 4. 服务端逻辑（Server‑Side Logic）
- 服务端主文件：server.py
  - ASR：使用 Whisper base model（加载见 server.py），用于将上传的清洗后音频转文本。
  - TTS：使用 Edge‑TTS（`edge_tts`）生成 MP3，再通过 FFmpeg 转码为 16kHz / 16bit / Mono WAV（`generate_tts_wav` 异步实现见 server.py）。
  - 音频处理流程（简要）：
    - 接收端上传音频 -> 保存为 raw_input.wav -> 使用 ffmpeg 转成 16k 单声道 -> Whisper 转录 -> LLM/回复生成 -> Edge‑TTS 生成 MP3 -> FFmpeg 转换为 16k WAV -> 返回并通过 `/get_audio/<filename>` 提供下载。

---

## 5. 交互流程（Interaction Loop）
流程与关键代码位置：
1. 唤醒检测（Snowboy） — 由 `IdleState::Update` 从 `AudioProcess` 读取帧并调用 Snowboy（模型由 `IdleState` 构造时加载）。
2. 播放提示音（Cue） — `ListeningState::Enter` 调用：`AudioProcess::PlayWavFile` 播放唤醒提示。
3. 录音（5s） — `ListeningState::Enter` / `Update` / `Exit` 调用：`AudioProcess::SaveStart` / `AudioProcess::SaveStop`（端侧零依赖 WAV 写入）。
4. 上传（POST /chat） — `ThinkingState::Update` 使用：`NetworkClient::SendAudio` 上传 `user_input.wav`。
5. Server (ASR/TTS) 处理 — 见 server.py。
6. 下载回复音频 — `NetworkClient::DownloadFile` 下载 `reply.wav`。
7. 播放回复 — `SpeakingState::Enter` 调用：`AudioProcess::PlayWavFile`；播放结束后由 `SpeakingState::Update` 切回 `IdleState`。

---

## 6. 模块功能速览（便于定位）
- [`AudioProcess`]AudioProcess.h / src/services/audio/AudioProcess.cc)  
  管理录音/播放线程、PCM 转换、WAV 写入/回填、队列、XRUN 恢复。
  - 关键方法：`AudioProcess::RecordLoop`, `AudioProcess::PlayLoop`, `AudioProcess::SaveStart`, `AudioProcess::SaveStop`, `AudioProcess::PlayWavFile`, `AudioProcess::ClearBuff`。

- [`NetworkClient`]NetworkClient.h / src/services/network/NetworkClient.cc)  
  libcurl 封装：上传（multipart）、下载（写文件回调）、请求超时、头部定制以兼容 Flask。
  - 关键方法：`NetworkClient::SendAudio`, `NetworkClient::DownloadFile`, `NetworkClient::SendRequest`。

- 状态机与流程（AI Chat）
  - 基类：`StateBase`  
  - 状态实现：`IdleState`, `ListeningState`, `ThinkingState`, `SpeakingState`  
  - 管理器：[`ChatApp`]chat_app.cc / src/app/AI_chat/chat_app.h) 与 `ChatContext`。

- Server：server.py（Whisper + Edge‑TTS + ffmpeg 转码）。

---

## 7. 针对嵌入式限制与兼容性的关键优化（要点）
- 零依赖端侧 WAV 写入，移除对 FFmpeg 的运行时依赖（降低镜像与运行期资源占用）：[`AudioProcess::SaveStart` / `SaveStop`](src/services/audio/AudioProcess.cc)。
- 单一音频管理进程（`AudioProcess`）独占设备，替换 `system("arecord"/"aplay")` 调用，避免设备竞用导致的阻塞/错误：speaking_state.cc 与 chat_app.cc 的相关改动。
- 网络兼容性：禁用分块/Expect 头，使用 Content-Length/MIME 上传以确保对 Flask 简单服务器的兼容：`NetworkClient::SendAudio`。
- 稳定性：录音链路的 XRUN 检测+pcm_prepare 恢复、播放端的单声道->双声道适配，提升在低资源/高负载条件下的鲁棒性：`AudioProcess::RecordLoop` / `AudioProcess::PlayLoop`。

---

## 参考源码（快速打开）
- 状态机与上下文
  - chat_app.h / chat_app.cc
  - chat_context.h
  - state_base.h
  - idle_state.h / idle_state.cc
  - listening_state.h / listening_state.cc
  - thinking_state.cc
  - speaking_state.cc

- 音频子系统
  - AudioProcess.h
  - AudioProcess.cc
  - TinyALSA 接口：include/tinyalsa/pcm.h
  - 实验/工具参考：tinycap.c, tinyplay.c

- 网络子系统
  - NetworkClient.h
  - NetworkClient.cc

- 服务端
  - server.py

---

如需我把上述文档导出为 PDF 或补充架构图（PlantUML / mermaid），或生成 CI 检查清单与回归测试用例，请告知所需格式及优先项。