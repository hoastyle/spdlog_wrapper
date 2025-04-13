/**
 * @file perf_config.cpp
 * @brief 性能测试配置相关函数实现
 */

#include "perf_config.h"
#include <iostream>

// 打印命令行帮助信息
void print_help() {
  std::cout << "MM-Logger Performance Test Tool" << std::endl;
  std::cout << "Usage: performance_test [options]" << std::endl;
  std::cout << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  --help               Show this help message" << std::endl;
  std::cout << "  --test=NAME          Test name (throughput, latency, stress, "
               "compare, all,"
            << std::endl;
  std::cout << "                         throughput_suite, latency_suite, "
               "stress_suite)"
            << std::endl;
  std::cout << "  --threads=N          Number of threads (default: 8)"
            << std::endl;
  std::cout << "  --iterations=N       Iterations per thread (default: 100000)"
            << std::endl;
  std::cout << "  --warmup=N           Warmup iterations (default: 10000)"
            << std::endl;
  std::cout << "  --use-mm-logger      Use mm_logger (default)" << std::endl;
  std::cout << "  --use-spdlog         Use spdlog directly for comparison"
            << std::endl;
  std::cout << "  --message-size=SIZE  Message size (small, medium, large; "
               "default: medium)"
            << std::endl;
  std::cout << "  --max-file-size=N    Max file size in MB (default: 10)"
            << std::endl;
  std::cout << "  --max-total-size=N   Max total size in MB (default: 100)"
            << std::endl;
  std::cout << "  --queue-size=N       Async queue size (default: 8192)"
            << std::endl;
  std::cout << "  --worker-threads=N   Worker threads (default: 2)"
            << std::endl;
  std::cout << "  --enable-console     Enable console output (default: "
               "disabled for performance)"
            << std::endl;
  std::cout << "  --disable-file       Disable file output" << std::endl;
  std::cout << "  --csv=FILE           Output CSV results to file" << std::endl;
  std::cout << "  --verbose            Enable verbose output" << std::endl;
  std::cout << std::endl;
  std::cout << "Examples:" << std::endl;
  std::cout << "  ./performance_test --test=throughput --threads=16 "
               "--iterations=50000"
            << std::endl;
  std::cout << "  ./performance_test --test=latency --threads=4 "
               "--iterations=10000 --message-size=small"
            << std::endl;
  std::cout
      << "  ./performance_test --test=compare --threads=8 --csv=results.csv"
      << std::endl;
  std::cout << "  ./performance_test --test=all --csv=full_results.csv"
            << std::endl;
}

// perf_config.cpp 中的参数解析修复

TestConfig parse_args(int argc, char* argv[]) {
  TestConfig config;
  config.test_name = "throughput";  // 默认测试

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "--help") {
      print_help();
      exit(0);
    } else if (arg.find("--test=") == 0) {
      config.test_name = arg.substr(7);
    } else if (arg.find("--threads=") == 0) {
      config.num_threads = std::stoi(arg.substr(10));
    } else if (arg.find("--iterations=") == 0) {
      config.iterations = std::stoi(arg.substr(13));
    } else if (arg.find("--warmup=") == 0) {
      config.warmup_iterations = std::stoi(arg.substr(9));
    } else if (arg == "--use-mm-logger") {
      config.use_mm_logger = true;
    } else if (arg == "--use-spdlog") {
      config.use_mm_logger = false;
    } else if (arg.find("--message-size=") == 0) {
      std::string size = arg.substr(15);
      if (size == "small") {
        config.message_size = MessageSize::SMALL;
        // 删除此行: config.test_name += "_msgsize_small";
      } else if (size == "medium") {
        config.message_size = MessageSize::MEDIUM;
        // 删除此行: config.test_name += "_msgsize_medium";
      } else if (size == "large") {
        config.message_size = MessageSize::LARGE;
        // 删除此行: config.test_name += "_msgsize_large";
      }
    } else if (arg.find("--max-file-size=") == 0) {
      config.max_file_size_mb = std::stoi(arg.substr(16));
    } else if (arg.find("--max-total-size=") == 0) {
      config.max_total_size_mb = std::stoi(arg.substr(17));
    } else if (arg.find("--queue-size=") == 0) {
      config.queue_size = std::stoi(arg.substr(13));
    } else if (arg.find("--worker-threads=") == 0) {
      config.worker_threads = std::stoi(arg.substr(17));
    } else if (arg == "--enable-console") {
      config.enable_console = true;
    } else if (arg == "--disable-file") {
      config.enable_file = false;
    } else if (arg.find("--csv=") == 0) {
      config.output_csv = true;
      config.csv_file   = arg.substr(6);
    } else if (arg == "--verbose") {
      config.verbose = true;
    }
  }

  return config;
}

// 获取消息大小名称
std::string get_message_size_name(MessageSize size) {
  switch (size) {
    case MessageSize::SMALL: return "Small";
    case MessageSize::MEDIUM: return "Medium";
    case MessageSize::LARGE: return "Large";
    default: return "Unknown";
  }
}
