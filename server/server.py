import os
import asyncio
import edge_tts
from flask import Flask, request, jsonify, send_file
import random
import whisper

app = Flask(__name__)

# 1. 路径配置
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
UPLOAD_FOLDER = os.path.join(BASE_DIR, 'uploads')
RESPONSE_FOLDER = os.path.join(BASE_DIR, 'responses')
os.makedirs(UPLOAD_FOLDER, exist_ok=True)
os.makedirs(RESPONSE_FOLDER, exist_ok=True)

# 2. 加载模型
print(">>> [Server] Loading Whisper model...")
# 你的电脑只有CPU，base模型够用了
asr_model = whisper.load_model("base")
print("✅ [Server] Whisper model loaded!")

async def generate_tts_wav(text, output_wav_file):
    print(f"   (TTS generating: {text}...)")
    mp3_file = output_wav_file.replace(".wav", ".mp3")
    
    # 语速稍微快一点 (+20%) 显得更自然
    communicate = edge_tts.Communicate(text, "zh-CN-XiaoxiaoNeural", rate="+20%")
    await communicate.save(mp3_file)
    
    # 转码
    cmd = f"ffmpeg -y -i \"{mp3_file}\" -acodec pcm_s16le -ar 16000 -ac 1 \"{output_wav_file}\" >/dev/null 2>&1"
    os.system(cmd)
    
    if os.path.exists(mp3_file): os.remove(mp3_file)

@app.route('/chat', methods=['POST'])
def chat():
    print("\n>>> [Server] New Request Received -----------------")
    
    # 1. 保存原始录音
    raw_path = os.path.join(UPLOAD_FOLDER, "raw_input.wav")
    clean_path = os.path.join(UPLOAD_FOLDER, "clean_input.wav")
    
    if 'audio' not in request.files:
        return jsonify({"error": "No audio"}), 400
        
    file = request.files['audio']
    file.save(raw_path)

    # ✅ [Debug] 检查文件大小，防止空文件导致 Crash
    raw_size = os.path.getsize(raw_path)
    print(f"   [File Info] Size: {raw_size} bytes")
    
    if raw_size == 0:
        print("❌ [Error] Received empty audio file from board!")
        return jsonify({"error": "Empty audio"}), 400

    # 2. ✅ [关键修复] 音频预处理
    # 强制转为 单声道(ac 1) + 16000Hz
    # 这能解决声道不匹配或噪音导致的幻觉问题
    cmd = f"ffmpeg -y -i \"{raw_path}\" -ac 1 -ar 16000 \"{clean_path}\" >/dev/null 2>&1"
    exit_code = os.system(cmd)
    
    if exit_code != 0:
        print("❌ [Error] FFmpeg failed! Please check if ffmpeg is installed.")
        return jsonify({"error": "FFmpeg error"}), 500

    # 3. [听] ASR 识别
    try:
        # initial_prompt 提示它可能是中文对话，language='zh' 强制中文
        result = asr_model.transcribe(
            clean_path, 
            language='zh',
            fp16=False  
        )
        user_text = result['text'].strip()
        print(f"✅ [ASR Result]: {user_text}")
    except Exception as e:
        print(f"❌ [ASR Error]: {e}")
        user_text = ""

    # 4. [想] 简单的回声逻辑 (下一步接大脑)
    if not user_text or len(user_text) < 2:
        ai_text = "抱歉，我没听清，请再说一遍。"
    else:
        ai_text = f"我听见你说了：{user_text}"

    print(f"   [AI Reply]: {ai_text}")

    # 5. [说] TTS
    reply_file = os.path.join(RESPONSE_FOLDER, 'reply.wav')
    try:
        asyncio.run(generate_tts_wav(ai_text, reply_file))
    except Exception as e:
        print(f"❌ [TTS Error]: {e}")
        return jsonify({"error": "TTS failed"}), 500

    return jsonify({
        "text": ai_text,
        "audio_url": "/get_audio/reply.wav"
    })

@app.route('/get_audio/<filename>', methods=['GET'])
def get_audio(filename):
    path = os.path.join(RESPONSE_FOLDER, filename)
    return send_file(path, mimetype="audio/wav")

if __name__ == '__main__':
    # 允许所有 IP 访问
    app.run(host='0.0.0.0', port=5000)