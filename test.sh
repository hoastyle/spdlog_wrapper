#!/bin/bash
# 运行日志轮转和多线程测试的脚本

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 确保日志目录存在
mkdir -p logs

# 编译所有测试程序
echo -e "${BLUE}编译测试程序...${NC}"
mkdir -p build
cd build
cmake ..
make
cd ..

# 清理之前的日志文件
echo -e "${YELLOW}清理之前的日志文件...${NC}"
rm -rf logs/*

# 运行基本日志示例
echo -e "${GREEN}运行基本日志示例...${NC}"
./build/examples/example
echo "基本示例完成"
ls -lh logs/

# 清理日志文件
rm -rf logs/*

# 运行日志轮转测试
echo -e "${GREEN}运行日志轮转测试...${NC}"
./build/tests/rotation_test 20000 40 1 5 5
echo "轮转测试完成"
ls -lh logs/

# 查看轮转测试结果
echo -e "${BLUE}查看日志文件和软链接:${NC}"
find logs -type f -name "*.rotation_test" | sort
find logs -type l -name "*.rotation_test*" | xargs ls -la 2>/dev/null

# 清理日志文件
echo -e "${YELLOW}清理日志文件...${NC}"
rm -rf logs/*

# 运行标准多线程测试
echo -e "${GREEN}运行标准多线程测试 (100线程/100迭代)...${NC}"
./build/examples/example_multithread --threads 100 --iterations 100
echo "标准多线程测试完成"
ls -lh logs/

# 清理日志文件
rm -rf logs/*

# 运行高负载多线程测试
echo -e "${GREEN}运行高负载多线程测试 (200线程/50迭代)...${NC}"
./build/examples/example_multithread --threads 200 --iterations 50 --worker-threads 4 --queue-size 16384
echo "高负载多线程测试完成"
ls -lh logs/

# 清理日志文件
rm -rf logs/*

# 运行无控制台输出的性能测试
echo -e "${GREEN}运行性能测试 (禁用控制台输出)...${NC}"
./build/examples/example_multithread --threads 50 --iterations 500 --no-console --no-delay --worker-threads 2 --queue-size 8192
echo "性能测试完成"
ls -lh logs/

echo -e "${BLUE}所有测试完成${NC}"
echo ""
echo -e "${YELLOW}可以通过以下命令运行更多测试:${NC}"
echo "./build/tests/rotation_test 50000 500 1 3 0    # 快速轮转测试，总大小限制3MB"
echo "./build/examples/example_multithread --help    # 查看多线程测试的所有选项"
echo "./build/examples/example_multithread --threads 300 --iterations 1000 --worker-threads 4 --queue-size 32768 --no-delay  # 超高负载测试"
