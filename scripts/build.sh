#作用：回到项目根目录，调用 CPU编译。
#!/bin/bash
set -e
cd "$(dirname "$0")/.."
make -j$(nproc)
