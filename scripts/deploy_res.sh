#!/bin/bash
# 这里的路径逻辑和 push.sh 一样，确保能找到项目根目录
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
if [ -d "$SCRIPT_DIR/product" ]; then PROJECT_ROOT="$SCRIPT_DIR"; elif [ -d "$SCRIPT_DIR/../product" ]; then PROJECT_ROOT="$SCRIPT_DIR/.."; else exit 1; fi
cd "$PROJECT_ROOT"

REMOTE_ROOT="/root/echo_mate"

echo "Deploying Static Resources..."

# 1. 推送 Snowboy
echo " -> Pushing Snowboy Models..."
adb shell "mkdir -p $REMOTE_ROOT/third_party/snowboy/resources"
adb push third_party/snowboy/resources/common.res $REMOTE_ROOT/third_party/snowboy/resources/
adb push third_party/snowboy/resources/snowboy.umdl $REMOTE_ROOT/third_party/snowboy/resources/

# 2. 推送 Assets (音频/图片)
# 你提到之前音频没推，这里补上，确保 assets 文件夹被推送到板子上
echo " -> Pushing Assets..."
adb shell "mkdir -p $REMOTE_ROOT/assets"
# 推送 assets 目录下所有文件
adb push assets/. $REMOTE_ROOT/assets/

echo "✅ Resources Deployed!"