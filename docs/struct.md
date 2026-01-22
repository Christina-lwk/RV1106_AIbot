.
├── Makefile                    # 项目主构建脚本，定义编译规则与链接参数
├── toolchain.mk                # 交叉编译工具链配置 (指定编译器、Sysroot路径)
├── assets/                     # 静态资源文件
│   └── sounds/                 # 系统提示音 (greeting.wav: 开机音, hm.wav: 唤醒反馈)
├── scripts/                    # 辅助脚本 (部署、运行、网络修复)
├── server/                     # 云端大脑 (Python 后端)
│   ├── server.py               # ★ 核心服务端逻辑：集成 Whisper(听)、DeepSeek(想)、Edge-TTS(说)
│   ├── uploads/                # 暂存设备上传的录音 (raw_input.wav)
│   └── responses/              # 暂存生成的回复音频 (reply.wav)
├── src/                        # 设备端固件源码 (C++)
│   ├── main.cc                 # ★ 系统入口 (Launcher)：负责 UI 刷新、Snowboy 唤醒检测、App 启动
│   ├── include/                # 通用头文件 (配置、日志宏定义)
│   ├── app/                    # 应用层逻辑
│   │   ├── AI_chat/            # [核心 App] 语音助手应用
│   │   │   ├── chat_app.cc     # App 控制器：管理状态机生命周期，响应 System 信号
│   │   │   ├── chat_context.h  # 上下文数据结构：在不同状态间共享数据 (如 should_exit 标志)
│   │   │   └── states/         # 有限状态机 (FSM) 实现
│   │   │       ├── listening_state.cc # 录音状态：实现 VAD (静音检测) 与 RMS 能量计算
│   │   │       ├── thinking_state.cc  # 思考状态：上传音频、解析服务端 JSON 指令
│   │   │       └── speaking_state.cc  # 说话状态：播放回复、非阻塞等待、决定是否退出
│   │   ├── app_manager.c       # App 管理器：负责 App 栈的切换 (Home <-> ChatApp)
│   │   └── home_app.c          # 默认主页 App (显示时钟/待机界面)
│   ├── services/               # 基础服务层 (单例模式)
│   │   ├── audio/              # 音频服务
│   │   │   └── AudioProcess.cc # ★ 核心音频引擎：双线程处理录音/播放，实现 WAV 头封装与软件声通分离
│   │   ├── network/            # 网络服务
│   │   │   └── NetworkClient.cc# HTTP 客户端：基于 libcurl 实现 Multipart 上传与 JSON 结果解析
│   │   └── wakeword/           # 唤醒服务
│   │   │   └── WakeWordEngine.cc # Snowboy 封装层：提供零拷贝检测接口
│   └── ui/                     # UI 适配层
│       └── lvgl_port.c         # LVGL 接口移植：显示驱动 (FBDEV) 与输入驱动 (EVDEV) 对接
└── third_party/                # 第三方依赖库
    ├── cjson/                  # 轻量级 JSON 解析库 (嵌入式专用)
    ├── lvgl/                   # 嵌入式 GUI 库
    ├── snowboy/                # 离线唤醒词检测库 (包含 .a 静态库与模型资源)
    └── tinyalsa/               # ALSA 音频接口封装 (用于底层 PCM 操作)