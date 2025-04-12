#!/bin/bash
# MM-Logger 性能测试执行脚本

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 创建日志目录
mkdir -p perf_logs

# 设置结果目录
RESULTS_DIR="./perf_results"
CSV_FILE="${RESULTS_DIR}/mm_logger_perf_results.csv"

# 确保结果目录存在
mkdir -p ${RESULTS_DIR}

# 清理之前的性能测试日志
echo -e "${YELLOW}清理之前的性能测试日志...${NC}"
rm -rf perf_logs/*

# 运行基本的比较测试
echo -e "${GREEN}运行基本比较测试 (MM-Logger vs 原生 spdlog)...${NC}"
./performance_test --test=compare --csv=${CSV_FILE}

# 运行高吞吐量测试套件
echo -e "${GREEN}运行吞吐量测试套件...${NC}"
./performance_test --test=throughput_suite --csv=${CSV_FILE} --disable-console

# 运行低延迟测试套件 (减少迭代次数以加快测试)
echo -e "${GREEN}运行延迟测试套件...${NC}"
./performance_test --test=latency_suite --iterations=5000 --csv=${CSV_FILE} --disable-console

# 运行压力测试套件
echo -e "${GREEN}运行压力测试套件...${NC}"
./performance_test --test=stress_suite --threads=16 --csv=${CSV_FILE} --disable-console

# 针对不同消息大小的比较测试
echo -e "${GREEN}运行不同消息大小的比较测试...${NC}"
for SIZE in small medium large; do
    echo -e "${BLUE}测试消息大小: ${SIZE}${NC}"
    ./performance_test --test=compare --message-size=${SIZE} \
        --csv=${CSV_FILE} --threads=8 --iterations=50000 --disable-console
done

# 针对不同线程配置的比较测试
echo -e "${GREEN}运行不同线程配置的比较测试...${NC}"
for THREADS in 1 4 16 32; do
    echo -e "${BLUE}测试线程数: ${THREADS}${NC}"
    ./performance_test --test=compare --threads=${THREADS} \
        --csv=${CSV_FILE} --iterations=50000 --disable-console
done

# 针对不同队列大小的比较测试
echo -e "${GREEN}运行不同队列大小的比较测试...${NC}"
for QUEUE in 1024 8192 32768; do
    echo -e "${BLUE}测试队列大小: ${QUEUE}${NC}"
    ./performance_test --test=compare --queue-size=${QUEUE} \
        --csv=${CSV_FILE} --threads=8 --iterations=50000 --disable-console
done

# 针对不同工作线程数的比较测试
echo -e "${GREEN}运行不同工作线程数的比较测试...${NC}"
for WORKERS in 1 2 4 8; do
    echo -e "${BLUE}测试工作线程数: ${WORKERS}${NC}"
    ./performance_test --test=compare --worker-threads=${WORKERS} \
        --csv=${CSV_FILE} --threads=8 --iterations=50000 --disable-console
done

echo -e "${BLUE}所有性能测试完成.${NC}"
echo ""
echo -e "${YELLOW}测试结果保存在: ${CSV_FILE}${NC}"
echo -e "${YELLOW}你可以使用 analyze_perf_results.py 生成图表和报告:${NC}"
echo -e "${GREEN}python analyze_perf_results.py ${CSV_FILE} ./perf_report${NC}"
