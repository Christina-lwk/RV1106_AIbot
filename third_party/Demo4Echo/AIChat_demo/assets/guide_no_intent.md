Collecting workspace informationFiltering to most relevant information### AIChat_demo 目录树结构图

```
AIChat_demo/
├── README.md                          # 项目说明文档
├── Client/                            # 客户端代码（C++实现）
│   ├── .gitignore                     # Git忽略文件
│   ├── CMakeLists.txt                 # CMake构建脚本
│   ├── main.cc                        # 客户端主入口
│   ├── README.md                      # 客户端说明文档
│   ├── toolchain.cmake                # 工具链配置
│   ├── Application/                   # 应用层核心逻辑
│   │   ├── Application.cc/.h          # 主应用类，管理整体流程
│   │   ├── StateConfig.cc/.h          # 状态机配置
│   │   ├── WS_Handler.cc/.h           # WebSocket消息处理器
│   │   ├── IntentsRegistry.cc/.h      # 意图注册管理
│   │   ├── UserStates/                # 用户定义的状态类
│   │   │   ├── Fault.cc/.h            # 故障状态
│   │   │   ├── Startup.cc/.h          # 启动状态
│   │   │   ├── Stop.cc/.h             # 停止状态
│   │   │   ├── Idle.cc/.h             # 空闲状态
│   │   │   ├── Listening.cc/.h        # 监听状态
│   │   │   ├── Thinking.cc/.h         # 思考状态
│   │   │   └── Speaking.cc/.h         # 说话状态
│   │   └── UserIntents/               # 用户定义的意图处理
│   │       └── RobotMove.cc/.h        # 机器人移动意图
│   ├── Audio/                         # 音频处理模块
│   │   └── AudioProcess.cc/.h         # 音频录制、播放和队列管理
│   ├── c_interface/                   # C接口层（供C代码调用）
│   │   ├── AIchat_c_interface.cc/.h   # C++到C的接口转换
│   ├── Events/                        # 事件管理
│   │   ├── AppEvents.h                # 应用事件定义
│   │   └── EventQueue.h               # 事件队列
│   ├── Intent/                        # 意图处理模块
│   │   └── IntentHandler.cc/.h        # 意图注册和执行
│   ├── StateMachine/                  # 状态机模块
│   │   └── StateMachine.cc/.h         # 状态机实现
│   ├── third_party/                   # 第三方库
│   │   └── snowboy/                   # 唤醒词检测库
│   ├── Utils/                         # 工具类
│   │   └── user_log.h                 # 日志工具
│   └── WebSocket/                     # WebSocket通信
│       └── WebsocketClient.cc/.h      # WebSocket客户端
└── Server/                            # 服务端代码（Python实现）
    ├── LICENSE                        # 许可证
    ├── main.py                        # 服务端主入口
    ├── README.md                      # 服务端说明文档
    └── ...                            # 其他服务端文件（未完全列出）
```

### 核心文件夹/文件的职责注释

- **Client/**: 客户端代码目录，负责语音交互、状态管理和WebSocket通信。
- **Client/Application/**: 应用层核心，封装业务逻辑，包括状态机、事件队列和意图处理。
  - **Application.cc/.h**: 主应用类，初始化组件，运行主循环，管理音频处理器、状态机和WebSocket客户端。
  - **StateConfig.cc/.h**: 配置状态机，注册状态和转换规则。
  - **WS_Handler.cc/.h**: 处理WebSocket接收的消息，解析JSON并触发事件。
  - **IntentsRegistry.cc/.h**: 注册意图函数，生成注册消息发送给服务端。
  - **UserStates/**: 各状态的具体实现，定义进入/退出动作和运行逻辑。
  - **UserIntents/**: 意图的具体执行逻辑，如机器人移动。
- **Client/Audio/**: 音频模块，处理录音、播放和数据队列。
- **Client/c_interface/**: C接口层，将C++类暴露为C函数，供外部（如DeskBot_demo）调用。
- **Client/Events/**: 事件系统，定义事件类型和队列，用于异步通信。
- **Client/Intent/**: 意图模块，注册和执行函数调用。
- **Client/StateMachine/**: 状态机实现，支持状态注册和事件驱动转换。
- **Client/WebSocket/**: WebSocket客户端，负责连接、发送/接收消息。
- **Server/**: 服务端代码，处理AI推理、语音合成等（Python实现）。

### 项目层级关系解释

项目采用分层架构：
- **顶层（Application层）**: `Application` 类作为核心，协调各模块。使用状态机管理生命周期，事件队列驱动异步操作。
- **中间层**: 状态机(`StateMachine`)、意图处理器(`IntentHandler`)、WebSocket处理器(`WSHandler`) 和音频处理器(`AudioProcess`) 作为独立模块，提供抽象接口。
- **底层**: 工具类(user_log.h)、事件队列(`EventQueue`) 和第三方库(snowboy) 支持基础功能。
- **接口层**: `c_interface` 提供C兼容接口，便于集成到C项目如DeskBot_demo。
- **服务端**: 与客户端通过WebSocket通信，处理AI任务，但不直接耦合客户端逻辑。

层级关系强调解耦：应用层依赖中间层，中间层依赖底层，接口层桥接C++和C。

### 核心业务流程描述

1. **初始化流程**:
   - main.cc 调用 `create_aichat_app` 创建 `Application` 实例。
   - `Application::Application` 初始化WebSocket客户端、音频处理器、状态机和事件队列。
   - `run_aichat_app` 调用 `Application::Run`，启动状态机和事件循环。

2. **语音交互流程**:
   - 空闲状态(`IdleState::Run`) 使用snowboy检测唤醒词，触发事件。
   - 状态机转换到监听状态(`ListeningState::Enter`)，启动录音。
   - 音频数据通过WebSocket发送到服务端，接收ASR/TTS结果。
   - WebSocket处理器(`WSHandler::ws_msg_handle`) 解析消息，入队事件。
   - 状态机转换到说话状态(`SpeakingState::Run`)，播放音频。

3. **意图处理流程**:
   - 服务端发送意图消息，`WSHandler::handle_intent_message` 解析并入队到 `IntentQueue_`。
   - `get_aichat_app_intent` 从队列取出，调用 `IntentHandler::HandleIntent` 执行注册函数，如 `RobotMove::Move`。

### 状态流转逻辑

状态机基于事件驱动，定义在 `StateConfig::Configure`。初始状态为startup，事件触发转换：
- startup → startup_done → idle
- idle → wake_detected → speaking (播放欢迎音频)
- speaking → tts_completed → idle (若对话未完成) 或 listening (若有语音输入)
- listening → vad_no_speech → idle
- 任意状态 → fault_happen → fault
- 任意状态 → to_stop → stopping

### 舍弃意图识别功能的影响

若舍弃意图识别，可删除：
- `Intent/` 目录及其文件（`IntentHandler.cc/.h`）。
- `Application/IntentsRegistry.cc/.h` 和 `Application/UserIntents/`。
- `Application::intent_handler_` 成员和相关初始化。
- `WSHandler::handle_intent_message` 方法。
- `IntentsRegistry::RegisterAllFunctions` 调用。
- `IntentQueue_` 和 `get_aichat_app_intent` 接口。
- 服务端相关意图处理代码（未提供，但假设类似）。这简化了客户端，移除函数注册和执行逻辑，但保留语音交互核心。