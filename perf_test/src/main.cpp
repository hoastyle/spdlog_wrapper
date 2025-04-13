/**
 * @file main.cpp
 * @brief MM-Logger性能测试工具主程序
 *
 * 此程序用于测试mm_logger库的性能特性，包括：
 * - 吞吐量测试（每秒可处理的日志量）
 * - 延迟测试（日志操作的响应时间）
 * - 压力测试（突发负载下的表现）
 * - 比较测试（与原生spdlog比较）
 */

#include "perf_config.h"
#include "perf_test.h"
#include "test_suites.h"
#include "utils.h"
#include <iostream>
#include <filesystem>

int main(int argc, char* argv[]) {
  // 解析命令行参数
  TestConfig config = parse_args(argc, argv);

  // 确保日志目录存在
  if (!utils::create_directory(config.log_dir)) {
    std::cerr << "Failed to create log directory: " << config.log_dir
              << std::endl;
    return 1;
  }

  std::cout << "MM-Logger Performance Test" << std::endl;
  std::cout << "Test: " << config.test_name << std::endl;
  std::cout << "Threads: " << config.num_threads << std::endl;
  std::cout << "Iterations: " << config.iterations << std::endl;
  std::cout << "Logger: " << (config.use_mm_logger ? "mm_logger" : "spdlog")
            << std::endl;
  std::cout << "Log directory: " << config.log_dir << std::endl;
  std::cout << std::endl;

  // 运行测试
  try {
    if (!run_single_test(config)) {
      std::cerr << "Unknown test: " << config.test_name << std::endl;
      print_help();
      return 1;
    }

    std::cout << "All tests completed successfully." << std::endl;
    if (config.output_csv) {
      std::cout << "Results written to " << config.csv_file << std::endl;
    }
  } catch (const std::exception& e) {
    std::cerr << "Error running tests: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
