import os
import asyncio
import edge_tts
from flask import Flask, request, jsonify, send_file
import random

app = Flask(__name__)

# [关键修改] 获取 server.py 文件所在的路径
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

# [关键修改] 所有的文件夹都基于 BASE_DIR 创建
UPLOAD_FOLDER = os.path.join(BASE_DIR, 'uploads')
RESPONSE_FOLDER = os.path.join(BASE_DIR, 'responses')

os.makedirs(UPLOAD_FOLDER, exist_ok=True)
os.makedirs(RESPONSE_FOLDER, exist_ok=True)

AI_RESPONSES = [
    "你好呀！我是你的 Echo Mate 机器人。",
    "网络连接成功了！我们可以聊天啦。",
    "虽然我现在还不够聪明，但我正在学习。",
    "你的声音真好听！",
    "收到，指令已确认。"
]

async def generate_tts_wav(text, output_wav_file):
    # 1. 先生成临时 MP3 文件
    mp3_file = output_wav_file.replace(".wav", ".mp3")
    communicate = edge_tts.Communicate(text, "zh-CN-XiaoxiaoNeural")
    await communicate.save(mp3_file)
    
    # 2. 调用 ffmpeg 转换为 WAV
    # 使用绝对路径，确保 ffmpeg 也能找到文件
    cmd = f"ffmpeg -y -i \"{mp3_file}\" -acodec pcm_s16le -ar 16000 -ac 1 \"{output_wav_file}\""
    os.system(cmd)
    
    # 3. 删除临时 MP3
    if os.path.exists(mp3_file):
        os.remove(mp3_file)

@app.route('/chat', methods=['POST'])
def chat():
    print("\n>>> [Server] 收到板子的请求！")
    
    if 'audio' in request.files:
        file = request.files['audio']
        file.save(os.path.join(UPLOAD_FOLDER, "user_input.wav"))
        print(">>> [Server] 已保存用户录音")

    ai_text = random.choice(AI_RESPONSES)
    print(f">>> [Server] AI 决定回复: {ai_text}")

    reply_file = os.path.join(RESPONSE_FOLDER, 'reply.wav')
    
    try:
        asyncio.run(generate_tts_wav(ai_text, reply_file))
        print(f">>> [Server] 语音合成并转码完毕: {reply_file}")
    except Exception as e:
        print(f">>> [Server] TTS 出错: {e}")
        return jsonify({"error": "TTS failed"}), 500

    return jsonify({
        "text": ai_text,
        "audio_url": "/get_audio/reply.wav"
    })

@app.route('/get_audio/<filename>', methods=['GET'])
def get_audio(filename):
    # 使用绝对路径读取文件
    path = os.path.join(RESPONSE_FOLDER, filename)
    print(f">>> [Server] 发送音频给板子: {path}")
    return send_file(path, mimetype="audio/wav")

if __name__ == '__main__':
    print(">>> [Server] 启动成功！")
    app.run(host='0.0.0.0', port=5000)