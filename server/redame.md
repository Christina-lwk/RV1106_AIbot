(后续做成md格式)
Server 的整体逻辑
现在的逻辑是一条清晰的 AI 对话流水线：

1.接收与清洗 (Receive & Clean):

板子上传录音 -> 服务器保存。

调用 ffmpeg 将其强制转换为 16kHz 单声道 WAV，消除可能存在的编码格式不兼容问题。

2.听 (ASR - Whisper):

加载 base 模型将清洗后的音频转为文本。

增加了 initial_prompt="以下是普通话的对话。"，这能显著减少 Whisper 将中文误识别为繁体或英文的情况。

3.想 (LLM - DeepSeek):

将文字发送给 DeepSeek API。

通过 system prompt 约束 AI：“你是嵌入式助手 Echo-Mate，回答要简短（50字内）”。这一点至关重要，因为如果 AI 回答几百字，TTS 生成和传输都会很慢，用户体验会很差。

4.说 (TTS - EdgeTTS):

将 AI 生成的回复文本转换为语音。

再次通过 ffmpeg 转码为 RV1106 硬件友好的格式（16k/16bit/Mono WAV），方便板子直接播放。

响应 (Response):

返回 JSON，包含回复文本（用于调试显示）和下载链接。