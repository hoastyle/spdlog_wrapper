/**
 * @file perf_config.h
 * @brief 性能测试配置和结果数据结构定义
 */

#pragma once

#include <string>
#include <vector>
#include <chrono>

// 日志消息大小枚举
enum class MessageSize {
  SMALL,   // ~64 bytes
  MEDIUM,  // ~256 bytes
  LARGE    // ~1024 bytes
};

// 测试配置结构
struct TestConfig {
  // 一般设置
  std::string test_name;
  std::string log_dir = "./perf_logs";
  std::string log_prefix;
  int num_threads       = 8;
  int iterations        = 100000;
  int warmup_iterations = 10000;
  bool use_mm_logger    = true;

  // 消息设置
  MessageSize message_size = MessageSize::MEDIUM;
  bool randomize_message   = false;

  // 日志器设置
  size_t max_file_size_mb  = 10;
  size_t max_total_size_mb = 100;
  bool enable_debug        = true;
  bool enable_console      = false;
  bool enable_file         = true;
  size_t queue_size        = 8192;
  size_t worker_threads    = 2;

  // 输出设置
  bool output_csv      = false;
  std::string csv_file = "performance_results.csv";
  bool verbose         = false;
};

// 性能结果结构
struct PerfResult {
  double total_time_ms   = 0;
  double logs_per_second = 0;
  std::vector<double> latencies_us;
  double min_latency_us    = 0;
  double max_latency_us    = 0;
  double median_latency_us = 0;
  double p95_latency_us    = 0;
  double p99_latency_us    = 0;
  size_t memory_used_kb    = 0;
};

// 解析命令行参数
TestConfig parse_args(int argc, char* argv[]);

// 打印帮助信息
void print_help();

// 获取消息大小名称
std::string get_message_size_name(MessageSize size);
