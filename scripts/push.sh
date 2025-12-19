#作用：找到程序 -> 瘦身-> 传到板子的 /tmp 目录。
#!/bin/bash
set -e
cd "$(dirname "$0")/.."

# 配置
APP="echo_mate_app"
LOCAL="product/bin/$APP"
REMOTE="/tmp"
STRIP="arm-rockchip830-linux-uclibcgnueabihf-strip"

if [ ! -f "$LOCAL" ]; then
    echo "Error: $LOCAL not found. Run build.sh first."
    exit 1
fi

# 瘦身并传输
cp $LOCAL ${LOCAL}_small
$STRIP ${LOCAL}_small
adb push ${LOCAL}_small $REMOTE/$APP
rm ${LOCAL}_small

echo "Push done: $REMOTE/$APP"
