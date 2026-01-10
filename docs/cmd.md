# Echo-Mate 开发常用指令清单

## 🚀 核心开发流程
在 Ubuntu 主机终端执行，用于日常编码后的部署。

| 命令 | 作用 | 备注 |
| :--- | :--- | :--- |
| **`em_build`** | **编译项目** | 每次修改 C/C++ 代码后必须执行 |
| **`em_push`** | **上传程序** | 编译后必须上传，新逻辑才会生效 |
| **`em_run`** | **运行程序** | 自动配置网络、路由、音量并启动 |

## 🛠️ 资源更新
当只修改了非代码资源时，无需重新编译，直接上传即可。

| 命令 | 作用 |
| :--- | :--- |
| `adb push assets/sounds/ /root/echo_mate/assets/` | **只更新音频** (如提示音) |
| `adb push assets/ /root/echo_mate/` | **更新所有资源** (图片/字体/音频) |

## 🩺 调试与体检 (ADB)
直接控制板子，排查“疑难杂症”。

| 命令 | 作用 | 正常表现 |
| :--- | :--- | :--- |
| `adb shell ifconfig` | 检查 IP | 应看到 `usb0` 有 `192.168.137.2` |
| `adb shell route` | 检查路由 | 应有 `default` 指向 `192.168.137.1` |
| `adb shell "ping -c 3 192.168.137.1"` | 测连通性 | 应该看到 `0% packet loss` |
| `adb shell ps \| grep echo_mate` | 查进程 | 如果卡死，会显示进程 ID |
| `adb shell killall echo_mate_app` | 杀进程 | 强制结束卡死的程序 |
| `adb shell "df -h"` | 查存储 | 查看 Flash 剩余空间 |

## 💻 服务器端 (Python)
AI 语音处理的大脑。

| 命令 | 作用 |
| :--- | :--- |
| `source venv/bin/activate` | 激活环境(在项目根目录下执行) |
| `python3 server/server.py` | 启动 AI 服务端 (监听 5000 端口) |
| `sudo ufw disable` | 如果连不上，尝试关闭防火墙 |

## 🛡️ 守护脚本
| 命令 | 作用 |
| :--- | :--- |
| `./scripts/auto_fix_net.sh` | **网络守护神**：自动修复 USB 掉线导致的 IP 丢失问题 |