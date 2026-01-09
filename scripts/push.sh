#!/bin/bash
set -e
cd "$(dirname "$0")/.."

# 配置
APP="echo_mate_app"
LOCAL="product/bin/$APP"
# [修正点] 目标目录改为和 run.sh 一致
REMOTE="/root/echo_mate/bin"
# [注意] 确保你的环境变量里有这个工具，如果没有，脚本会自动跳过瘦身
STRIP="arm-rockchip830-linux-uclibcgnueabihf-strip"

if [ ! -f "$LOCAL" ]; then
    echo "Error: $LOCAL not found. Run build.sh first."
    exit 1
fi

# 尝试瘦身 (如果工具存在)
if command -v $STRIP >/dev/null 2>&1; then
    echo "Stripping binary..."
    cp $LOCAL ${LOCAL}_small
    $STRIP ${LOCAL}_small
    target_file="${LOCAL}_small"
else
    echo "Strip tool not found, pushing original size..."
    target_file="$LOCAL"
fi

# 确保板子上目录存在 (防止报错)
adb shell "mkdir -p $REMOTE"

# 传输
echo "Pushing to $REMOTE..."
adb push $target_file $REMOTE/$APP

# 清理临时文件
if [ "$target_file" != "$LOCAL" ]; then
    rm $target_file
fi

echo "✅ Push done: $REMOTE/$APP"