/**
 * @file perf_test.h
 * @brief 性能测试类定义
 */

#pragma once

#include "perf_config.h"
#include <string>
#include <mutex>
#include <condition_variable>

// 线程同步栅栏，确保所有线程同时开始测试
class ThreadBarrier {
 public:
  explicit ThreadBarrier(size_t count);
  void wait();

 private:
  std::mutex mutex_;
  std::condition_variable cv_;
  size_t threshold_;
  size_t count_;
  size_t generation_;
};

// 性能测试类
class PerformanceTest {
 public:
  explicit PerformanceTest(const TestConfig& config);
  ~PerformanceTest();

  // 禁止拷贝
  PerformanceTest(const PerformanceTest&)            = delete;
  PerformanceTest& operator=(const PerformanceTest&) = delete;

  // 测试方法
  PerfResult run_throughput_test();
  PerfResult run_latency_test();
  PerfResult run_stress_test(int burst_size, int burst_count);

  // 获取内存使用量 (KB)
  size_t get_memory_usage();

  // 结果输出方法
  void print_results(const PerfResult& result, const std::string& test_type);
  void write_csv_results(
      const PerfResult& result, const std::string& test_type);

 private:
  // 初始化日志系统
  void init_mm_logger();
  void init_spdlog();

  // 生成测试消息
  void generate_test_messages();

  // 记录一条日志消息
  void log_message(int thread_id, int iteration, bool flush);

  TestConfig config_;
  std::string test_message_;
};
