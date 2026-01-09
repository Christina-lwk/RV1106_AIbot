ai_chat                # [新] AI 聊天机器人独立模块
    ├── chat_app.cc        # 聊天应用的入口 (管理 Context 和主循环)
    ├── chat_app.h
    ├── chat_context.h     # 聊天专用的上下文 (持有 Audio, Network指针)
    └── states             # [新] 聊天专属的状态机
        ├── state_base.h   # 状态基类
        ├── idle_state.h   # 待机/唤醒监听
        ├── listening.h    # 录音中
        └── speaking.h     # 说话中