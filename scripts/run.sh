#!/bin/bash
echo "Configuring Board Network & Starting App..."

# 网络配置和启动命令
adb shell "ifconfig usb0 192.168.137.2 up && \
           (route add default gw 192.168.137.1 dev usb0 || true) && \
           cd /root/echo_mate/bin && \
           ./echo_mate_app"