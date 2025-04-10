#!/bin/bash
# 运行日志轮转测试的脚本

# 确保日志目录存在
mkdir -p logs

# 编译测试程序
echo "编译测试程序..."
cd build
make rotation_test
cd ..

# 运行测试
echo "开始日志轮转测试..."

# 使用参数运行测试
# 参数: [日志条数] [每条大小] [单文件大小MB] [总大小限制MB] [间隔毫秒]
./build/tests/rotation_test 20000 200 1 5 5

echo "测试完成，检查日志文件轮转情况..."
ls -lh logs/

echo "查看日志文件和软链接:"
find logs -type f -name "*.rotation_test" | sort
find logs -type l -name "*.rotation_test*" | xargs ls -la

echo ""
echo "可以通过不同参数运行测试，例如:"
echo "./build/tests/rotation_test 50000 500 1 3 0    # 快速测试，总大小限制3MB"
echo "./build/tests/rotation_test 10000 200 2 10 10  # 模拟真实场景，单文件2MB，总大小10MB"
echo "./build/tests/rotation_test 100000 100 5 20 100 # 长时间运行测试，单文件5MB，总大小20MB"
