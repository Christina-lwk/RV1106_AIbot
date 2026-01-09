#!/bin/bash
echo "🔍 开始智能监控 USB 网卡..."
echo "按 Ctrl+C 停止"

while true; do
    # 1. 自动寻找名字以 'enx' 开头的网卡 (通常是 USB 网卡)
    # 排除掉 lo (本地) 和 ens33 (虚拟机主网卡)
    INTERFACE=$(ip link show | grep 'enx' | awk -F': ' '{print $2}')

    if [ -z "$INTERFACE" ]; then
        echo -n "." # 没找到网卡，打印个点
    else
        # 2. 检查这个网卡有没有 IP
        HAS_IP=$(ip addr show $INTERFACE | grep "192.168.137.1")

        if [ -z "$HAS_IP" ]; then
            echo ""
            echo "⚡ 发现新网卡: $INTERFACE (MAC变了!)"
            echo "🚀 正在强制赋予 IP: 192.168.137.1..."
            
            # 强制设 IP
            sudo ifconfig $INTERFACE 192.168.137.1 netmask 255.255.255.0 up
            
            echo "✅ 电脑端 IP 已修复。快去跑板子程序！"
        fi
    fi
    
    # 每秒检查一次
    sleep 1
done