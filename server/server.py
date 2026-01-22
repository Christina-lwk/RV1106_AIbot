import os
import asyncio
import edge_tts
from flask import Flask, request, jsonify, send_file
import whisper
from openai import OpenAI
from dotenv import load_dotenv

# 1. 加载环境变量
load_dotenv()

app = Flask(__name__)

# --- 配置区域 ---
API_KEY = os.getenv("DEEPSEEK_API_KEY")
BASE_URL = "https://api.deepseek.com"

if not API_KEY:
    print("❌ [Error] API Key not found! Please check your .env file.")
    exit(1)

client = OpenAI(api_key=API_KEY, base_url=BASE_URL)

# 路径配置
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
UPLOAD_FOLDER = os.path.join(BASE_DIR, 'uploads')
RESPONSE_FOLDER = os.path.join(BASE_DIR, 'responses')
os.makedirs(UPLOAD_FOLDER, exist_ok=True)
os.makedirs(RESPONSE_FOLDER, exist_ok=True)

print(">>> [Server] Loading Whisper model...")
asr_model = whisper.load_model("base")
print("✅ [Server] Whisper model loaded!")

# --- [优化 2] 对话历史记录 ---
# 用于存储上下文 [{"role": "user", "content": "..."}, ...]
chat_history = []
MAX_HISTORY_TURNS = 20  # 限制保留最近 20 轮，防止 token 爆炸

# 系统提示词
SYSTEM_PROMPT = {
    "role": "system", 
    "content": (
        "你是一个运行在嵌入式设备上的语音助手 Echo-Mate。"
        "请用中文回答，风格亲切、简练。"
        "因为通过语音播报，请务必控制回答在 50 字以内。"
    )
}

# --- 核心功能函数 ---

def ask_deepseek(text):
    """调用 DeepSeek API 获取回复 (带上下文)"""
    global chat_history
    
    print(f"   [Thinking] User asked: {text}")

    # 1. 构建当前请求的消息列表
    # 输入 = System Prompt+chat_history+当前用户输入
    messages = [SYSTEM_PROMPT] + chat_history 
    messages.append({"role": "user", "content": text})

    try:
        response = client.chat.completions.create(
            model="deepseek-chat",
            messages=messages,
            stream=False
        )
        reply = response.choices[0].message.content
        
        # 2. 更新历史记录
        chat_history.append({"role": "user", "content": text})
        chat_history.append({"role": "assistant", "content": reply})
        
        # 3. 保持记忆在限制范围内 (FIFO)
        if len(chat_history) > MAX_HISTORY_TURNS * 2:
            # 切片保留最后 N 条
            chat_history = chat_history[-(MAX_HISTORY_TURNS * 2):]

        return reply
    except Exception as e:
        print(f"❌ [LLM Error]: {e}")
        return "抱歉，我的大脑连接暂时断开了。"

async def generate_tts_wav(text, output_wav_file):
    """生成 TTS 音频并转码"""
    mp3_file = output_wav_file.replace(".wav", ".mp3")

    # 语速 +20% 进一步加快响应感
    communicate = edge_tts.Communicate(text, "zh-CN-XiaoxiaoNeural", rate="+20%")
    await communicate.save(mp3_file)
    
    cmd = f'ffmpeg -y -i "{mp3_file}" -acodec pcm_s16le -ar 16000 -ac 1 "{output_wav_file}" >/dev/null 2>&1'
    os.system(cmd)
    
    if os.path.exists(mp3_file):
        os.remove(mp3_file)

# --- Flask 路由 ---

@app.route('/chat', methods=['POST'])
def chat():
    global chat_history
    print("\n>>> [Server] New Request -----------------")
    
    raw_path = os.path.join(UPLOAD_FOLDER, "raw_input.wav")
    clean_path = os.path.join(UPLOAD_FOLDER, "clean_input.wav")
    reply_file = os.path.join(RESPONSE_FOLDER, 'reply.wav')
    
    if 'audio' not in request.files:
        return jsonify({"error": "No audio"}), 400
    
    file = request.files['audio']
    file.save(raw_path)

    if os.path.getsize(raw_path) == 0:
        return jsonify({"error": "Empty audio"}), 400

    cmd = f'ffmpeg -y -i "{raw_path}" -ac 1 -ar 16000 "{clean_path}" >/dev/null 2>&1'
    os.system(cmd)

    try:
        # tiny 模型对 initial_prompt 更加敏感，这行很重要
        result = asr_model.transcribe(clean_path, language='zh', initial_prompt="你好")
        user_text = result['text'].strip()
        print(f"   [Heard]: {user_text}")
    except Exception as e:
        print(f"❌ [ASR Error]: {e}")
        user_text = ""

    # 退出意图检测
    should_end_session = False
    exit_keywords = ["再见", "拜拜", "退出", "退下", "闭嘴", "休息"]
    
    # 如果检测到退出，或者用户什么都没说(幻听处理)
    if any(keyword in user_text for keyword in exit_keywords):
        should_end_session = True
        print("✅ [Intent] Exit keyword detected. Clearing history.")
        chat_history = [] # 清空记忆
        ai_text = "好的，下次见。"
    
    elif not user_text or len(user_text) < 1:
        # 如果什么都没听见，不要去请求 DeepSeek (浪费时间且污染历史)
        ai_text = "我没听清。"
    else:
        # 正常对话
        ai_text = ask_deepseek(user_text)
    
    print(f"   [Reply]: {ai_text}")

    try:
        asyncio.run(generate_tts_wav(ai_text, reply_file))
    except Exception as e:
        print(f"❌ [TTS Error]: {e}")
        return jsonify({"error": "TTS failed"}), 500

    return jsonify({
        "text": ai_text,
        "audio_url": "/get_audio/reply.wav",
        "should_end_session": should_end_session
    })

@app.route('/get_audio/<filename>', methods=['GET'])
def get_audio(filename):
    path = os.path.join(RESPONSE_FOLDER, filename)
    if os.path.exists(path):
        return send_file(path, mimetype="audio/wav")
    else:
        return "File not found", 404

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)