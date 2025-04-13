#!/bin/bash
# 增强版MM-Logger性能测试执行脚本

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 日志函数
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_title() {
    echo -e "\n${CYAN}========== $1 ==========${NC}"
}

# 创建日志目录
mkdir -p perf_logs

# 设置结果目录
RESULTS_DIR="./perf_results"
CSV_FILE="${RESULTS_DIR}/mm_logger_perf_results.csv"

# 确保结果目录存在
mkdir -p ${RESULTS_DIR}

# 清理之前的性能测试日志
log_warning "清理之前的性能测试日志..."
rm -rf perf_logs/*

# 基本设置
BASE_ITERATIONS=10000
REDUCED_ITERATIONS=5000
THREADS_DEFAULT=2
CONSOLE_OUTPUT="--disable-console" # 默认禁用控制台输出以提高性能

# 运行单个测试的函数
run_test() {
    NAME=$1
    shift
    log_info "运行测试: $NAME"
    echo -e "${GREEN}./performance_test $@${NC}"
    ./performance_test $@

    # 检查执行结果
    if [ $? -ne 0 ]; then
        log_error "测试失败: $NAME"
    else
        log_success "测试完成: $NAME"
    fi

    # 短暂延迟确保资源释放
    sleep 2
}

# 1. 基本测试套件
log_title "基本测试套件"

# 基本吞吐量测试
run_test "基本吞吐量测试" \
    --test=throughput \
    --csv=${CSV_FILE} \
    --iterations=${BASE_ITERATIONS} \
    --threads=${THREADS_DEFAULT} \
    ${CONSOLE_OUTPUT}

# 基本延迟测试
run_test "基本延迟测试" \
    --test=latency \
    --csv=${CSV_FILE} \
    --iterations=${REDUCED_ITERATIONS} \
    --threads=${THREADS_DEFAULT} \
    ${CONSOLE_OUTPUT}

# 基本压力测试
run_test "基本压力测试" \
    --test=stress \
    --csv=${CSV_FILE} \
    --iterations=${REDUCED_ITERATIONS} \
    --threads=${THREADS_DEFAULT} \
    ${CONSOLE_OUTPUT}

# 2. 线程扩展性测试
log_title "线程扩展性测试"

# 测试不同数量的线程
for THREADS in 1 2 4 8 16; do
    TEST_NAME="吞吐量测试-${THREADS}线程"
    run_test "${TEST_NAME}" \
        --test=throughput \
        --threads=${THREADS} \
        --iterations=${REDUCED_ITERATIONS} \
        --csv=${CSV_FILE} \
        ${CONSOLE_OUTPUT}
done

# 3. 消息大小影响测试
log_title "消息大小影响测试"

# 测试不同的消息大小 - MM-Logger
for SIZE in small medium large; do
    TEST_NAME="吞吐量测试-${SIZE}消息-mm_logger"
    run_test "${TEST_NAME}" \
        --test=throughput \
        --message-size=${SIZE} \
        --threads=${THREADS_DEFAULT} \
        --iterations=${REDUCED_ITERATIONS} \
        --csv=${CSV_FILE} \
        --use-mm-logger \
        ${CONSOLE_OUTPUT}
done

# 测试不同的消息大小 - spdlog
for SIZE in small medium large; do
    TEST_NAME="吞吐量测试-${SIZE}消息-spdlog"
    run_test "${TEST_NAME}" \
        --test=throughput \
        --message-size=${SIZE} \
        --threads=${THREADS_DEFAULT} \
        --iterations=${REDUCED_ITERATIONS} \
        --csv=${CSV_FILE} \
        --use-spdlog \
        ${CONSOLE_OUTPUT}
done

# 4. 异步队列配置测试
log_title "异步队列配置测试"

# 测试不同的队列大小
for QUEUE in 1024 4096 8192 16384 32768; do
    TEST_NAME="吞吐量测试-队列大小${QUEUE}"
    run_test "${TEST_NAME}" \
        --test=throughput \
        --queue-size=${QUEUE} \
        --threads=${THREADS_DEFAULT} \
        --iterations=${REDUCED_ITERATIONS} \
        --csv=${CSV_FILE} \
        ${CONSOLE_OUTPUT}
done

# 测试不同数量的工作线程
for WORKERS in 1 2 4 8; do
    TEST_NAME="吞吐量测试-${WORKERS}工作线程"
    run_test "${TEST_NAME}" \
        --test=throughput \
        --worker-threads=${WORKERS} \
        --threads=${THREADS_DEFAULT} \
        --iterations=${REDUCED_ITERATIONS} \
        --csv=${CSV_FILE} \
        ${CONSOLE_OUTPUT}
done

# 5. 控制台输出影响测试
log_title "控制台输出影响测试"

# 测试启用控制台
run_test "吞吐量测试-启用控制台" \
    --test=throughput \
    --threads=${THREADS_DEFAULT} \
    --iterations=${REDUCED_ITERATIONS} \
    --csv=${CSV_FILE} \
    --enable-console

# 测试禁用控制台
run_test "吞吐量测试-禁用控制台" \
    --test=throughput \
    --threads=${THREADS_DEFAULT} \
    --iterations=${REDUCED_ITERATIONS} \
    --csv=${CSV_FILE} \
    --disable-console

# 6. 文件轮转配置测试
log_title "文件轮转配置测试"

# 测试不同的单文件大小限制
for FILE_SIZE in 1 5 10 50; do
    TEST_NAME="吞吐量测试-文件大小${FILE_SIZE}MB"
    run_test "${TEST_NAME}" \
        --test=throughput \
        --max-file-size=${FILE_SIZE} \
        --threads=${THREADS_DEFAULT} \
        --iterations=${REDUCED_ITERATIONS} \
        --csv=${CSV_FILE} \
        ${CONSOLE_OUTPUT}
done

# 测试不同的总文件大小限制
for TOTAL_SIZE in 10 50 100 200; do
    TEST_NAME="吞吐量测试-总大小${TOTAL_SIZE}MB"
    run_test "${TEST_NAME}" \
        --test=throughput \
        --max-total-size=${TOTAL_SIZE} \
        --threads=${THREADS_DEFAULT} \
        --iterations=${REDUCED_ITERATIONS} \
        --csv=${CSV_FILE} \
        ${CONSOLE_OUTPUT}
done

# 7. 密集压力测试
log_title "密集压力测试"

# 小型突发测试
run_test "压力测试-小型突发" \
    --test=stress \
    --threads=4 \
    --iterations=1000 \
    --csv=${CSV_FILE} \
    ${CONSOLE_OUTPUT}

# 中型突发测试
run_test "压力测试-中型突发" \
    --test=stress \
    --threads=8 \
    --iterations=500 \
    --csv=${CSV_FILE} \
    ${CONSOLE_OUTPUT}

# 大型突发测试
run_test "压力测试-大型突发" \
    --test=stress \
    --threads=16 \
    --iterations=200 \
    --csv=${CSV_FILE} \
    ${CONSOLE_OUTPUT}

# 8. 高并发场景测试
log_title "高并发场景测试"

# 高线程数+小消息
run_test "高并发-小消息" \
    --test=throughput \
    --threads=16 \
    --message-size=small \
    --iterations=1000 \
    --csv=${CSV_FILE} \
    ${CONSOLE_OUTPUT}

# 高线程数+大队列
run_test "高并发-大队列" \
    --test=throughput \
    --threads=16 \
    --queue-size=32768 \
    --iterations=1000 \
    --csv=${CSV_FILE} \
    ${CONSOLE_OUTPUT}

# 高线程数+多工作线程
run_test "高并发-多工作线程" \
    --test=throughput \
    --threads=16 \
    --worker-threads=8 \
    --iterations=1000 \
    --csv=${CSV_FILE} \
    ${CONSOLE_OUTPUT}

# 9. 延迟敏感场景测试
log_title "延迟敏感场景测试"

# 线程数对延迟的影响
for THREADS in 1 2 4 8; do
    TEST_NAME="延迟测试-${THREADS}线程"
    run_test "${TEST_NAME}" \
        --test=latency \
        --threads=${THREADS} \
        --iterations=2000 \
        --csv=${CSV_FILE} \
        ${CONSOLE_OUTPUT}
done

# 消息大小对延迟的影响
for SIZE in small medium large; do
    TEST_NAME="延迟测试-${SIZE}消息"
    run_test "${TEST_NAME}" \
        --test=latency \
        --message-size=${SIZE} \
        --threads=${THREADS_DEFAULT} \
        --iterations=2000 \
        --csv=${CSV_FILE} \
        ${CONSOLE_OUTPUT}
done

# 10. MM-Logger vs spdlog 全面对比
log_title "MM-Logger vs spdlog 全面对比"

# 修复：分开运行比较测试
run_test "基础对比测试" \
    --test=compare \
    --threads=${THREADS_DEFAULT} \
    --iterations=5000 \
    --csv=${CSV_FILE} \
    ${CONSOLE_OUTPUT}

# 高线程数对比
run_test "高线程数对比" \
    --test=compare \
    --threads=8 \
    --iterations=2000 \
    --csv=${CSV_FILE} \
    ${CONSOLE_OUTPUT}

# 大消息对比
run_test "大消息对比" \
    --test=compare \
    --message-size=large \
    --threads=${THREADS_DEFAULT} \
    --iterations=2000 \
    --csv=${CSV_FILE} \
    ${CONSOLE_OUTPUT}

# 突发负载对比
run_test "突发负载对比" \
    --test=stress \
    --threads=4 \
    --use-mm-logger \
    --iterations=2000 \
    --csv=${CSV_FILE} \
    ${CONSOLE_OUTPUT}

run_test "突发负载对比" \
    --test=stress \
    --threads=4 \
    --use-spdlog \
    --iterations=2000 \
    --csv=${CSV_FILE} \
    ${CONSOLE_OUTPUT}

log_title "性能测试总结"
log_success "所有性能测试完成."
log_info "测试结果保存在: ${CSV_FILE}"
log_info "你可以使用 analyze_perf_results.py 生成图表和报告:"
echo -e "${GREEN}python analyze_perf_results.py ${CSV_FILE} ./perf_report${NC}"

# 尝试自动生成报告（如果脚本存在）
if [ -f "./analyze_perf_results.py" ]; then
    log_title "生成性能报告"
    python analyze_perf_results.py ${CSV_FILE} ./perf_report
    log_success "报告已生成，位于 ./perf_report 目录"
fi
