#!/bin/bash
set -e
# 获取脚本所在目录的绝对路径逻辑 (保持你现有的)
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
if [ -d "$SCRIPT_DIR/product" ]; then
    PROJECT_ROOT="$SCRIPT_DIR"
elif [ -d "$SCRIPT_DIR/../product" ]; then
    PROJECT_ROOT="$SCRIPT_DIR/.."
else
    echo "❌ Error: Cannot locate project root!"
    exit 1
fi
cd "$PROJECT_ROOT"

# ================= 配置 =================
APP="echo_mate_app"
LOCAL_BIN="product/bin/$APP"
REMOTE_ROOT="/root/echo_mate"

STRIP="arm-rockchip830-linux-uclibcgnueabihf-strip"
# =======================================

# 1. 编译检查
if [ ! -f "$LOCAL_BIN" ]; then
    echo "Error: File '$LOCAL_BIN' not found! Run 'make' first."
    exit 1
fi

# 2. 瘦身
if command -v $STRIP >/dev/null 2>&1; then
    cp "$LOCAL_BIN" "${LOCAL_BIN}_small"
    $STRIP "${LOCAL_BIN}_small"
    target_file="${LOCAL_BIN}_small"
else
    target_file="$LOCAL_BIN"
fi

# 3. 这里的 mkdir 保留，防止误删目录
adb shell "mkdir -p $REMOTE_ROOT"

# 4. 只推 App (速度极快)
echo "Pushing App..."
adb push "$target_file" "$REMOTE_ROOT/$APP"
adb shell "chmod +x $REMOTE_ROOT/$APP"

if [ "$target_file" != "$LOCAL_BIN" ]; then
    rm "$target_file"
fi

echo "✅ App Update Done!"