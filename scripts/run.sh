#作用：给权限，运行。
#!/bin/bash

APP="echo_mate_app"
REMOTE="/tmp"

# 远程执行：加权限 && 运行
adb shell "chmod +x $REMOTE/$APP && $REMOTE/$APP"
