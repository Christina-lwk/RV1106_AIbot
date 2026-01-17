import os
import asyncio
import edge_tts
from flask import Flask, request, jsonify, send_file
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
asr_model = whisper.load_model("base")
print("✅ [Server] Whisper model loaded!")

# --- [核心修复] TTS 生成函数 ---
async def generate_tts_wav(text, output_wav_file):
    # 临时 MP3 文件名
    mp3_file = output_wav_file.replace(".wav", ".mp3")
    
    # 1. 生成 MP3
    communicate = edge_tts.Communicate(text, "zh-CN-XiaoxiaoNeural", rate="+20%")
    await communicate.save(mp3_file)
    
    # 2. 转码: MP3 -> WAV (16k, 单声道, S16LE)
    cmd = f'ffmpeg -y -i "{mp3_file}" -acodec pcm_s16le -ar 16000 -ac 1 "{output_wav_file}" >/dev/null 2>&1'
    os.system(cmd)
    
    # 清理临时文件
    if os.path.exists(mp3_file):
        os.remove(mp3_file)

@app.route('/chat', methods=['POST'])
def chat():
    print("\n>>> [Server] New Request Received -----------------")
    
    raw_path = os.path.join(UPLOAD_FOLDER, "raw_input.wav")
    clean_path = os.path.join(UPLOAD_FOLDER, "clean_input.wav")
    
    # 1. 接收文件
    if 'audio' not in request.files:
        return jsonify({"error": "No audio"}), 400
    
    file = request.files['audio']
    file.save(raw_path)

    # 检查文件大小
    if os.path.getsize(raw_path) == 0:
        print("❌ [Error] Received empty audio file!")
        return jsonify({"error": "Empty audio"}), 400

    # 2. 音频清洗 (WAV -> WAV)
    # 既然板子现在上传的是标准 WAV，我们可以让 ffmpeg 自动识别输入格式
    # 强制输出为 16k 单声道，确保 Whisper 听得清
    cmd = f'ffmpeg -y -i "{raw_path}" -ac 1 -ar 16000 "{clean_path}" >/dev/null 2>&1'
    os.system(cmd)

    # 3. [听] Whisper 识别
    try:
        # initial_prompt 提示它是中文
        result = asr_model.transcribe(clean_path, language='zh', fp16=False)
        user_text = result['text'].strip()
        print(f"✅ [ASR Result]: {user_text}")
    except Exception as e:
        print(f"❌ [ASR Error]: {e}")
        user_text = ""

    # 4. [想] 简单的对话逻辑
    if not user_text or len(user_text) < 2:
        ai_text = "[Missed]抱歉，我没听清。"
    else:
        ai_text = f"我听见你说了：{user_text}"
    
    print(f"   [AI Reply]: {ai_text}")

    # 5. [说] TTS 生成
    reply_file = os.path.join(RESPONSE_FOLDER, 'reply.wav')
    try:
        asyncio.run(generate_tts_wav(ai_text, reply_file))
    except Exception as e:
        print(f"❌ [TTS Error]: {e}")
        return jsonify({"error": "TTS failed"}), 500

    # 返回给板子
    return jsonify({
        "text": ai_text,
        "audio_url": "/get_audio/reply.wav"
    })

@app.route('/get_audio/<filename>', methods=['GET'])
def get_audio(filename):
    path = os.path.join(RESPONSE_FOLDER, filename)
    return send_file(path, mimetype="audio/wav")

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)