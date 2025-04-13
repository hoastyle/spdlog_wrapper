/**
 * @file test_suites.cpp
 * @brief 测试套件函数实现
 */

#include "test_suites.h"
#include "perf_test.h"
#include "utils.h"
#include <iostream>
#include <vector>
#include <stdexcept>

// 运行吞吐量测试套件
void run_throughput_test_suite(TestConfig base_config) {
  try {
    // 基本配置设置
    std::string original_name = base_config.test_name;
    base_config.test_name     = "throughput_suite";

    std::cout << "Running throughput test suite..." << std::endl;

    // 不同线程数的测试
    std::vector<int> thread_counts = {1, 2, 4, 8};  // 减少线程数以避免资源问题
    for (int threads : thread_counts) {
      try {
        TestConfig config  = base_config;
        config.test_name   = "throughput_threads_" + std::to_string(threads);
        config.num_threads = threads;

        PerformanceTest test(config);
        PerfResult result     = test.run_throughput_test();
        result.memory_used_kb = test.get_memory_usage();

        test.print_results(result,
            "Throughput Test (" + std::to_string(threads) + " threads)");

        if (config.output_csv) {
          test.write_csv_results(result, "throughput");
        }
      } catch (const std::exception& e) {
        std::cerr << "Error in thread count test (" << threads
                  << "): " << e.what() << std::endl;
        continue;  // 跳过失败的测试，继续执行其他测试
      }
    }

    // 不同消息大小的测试
    std::vector<MessageSize> message_sizes = {
        MessageSize::SMALL, MessageSize::MEDIUM, MessageSize::LARGE};
    for (MessageSize size : message_sizes) {
      try {
        TestConfig config = base_config;

        std::string size_name;
        switch (size) {
          case MessageSize::SMALL: size_name = "small"; break;
          case MessageSize::MEDIUM: size_name = "medium"; break;
          case MessageSize::LARGE: size_name = "large"; break;
        }

        config.test_name    = "throughput_msgsize_" + size_name;
        config.message_size = size;

        PerformanceTest test(config);
        PerfResult result     = test.run_throughput_test();
        result.memory_used_kb = test.get_memory_usage();

        test.print_results(
            result, "Throughput Test (" + size_name + " messages)");

        if (config.output_csv) {
          test.write_csv_results(result, "throughput");
        }
      } catch (const std::exception& e) {
        std::cerr << "Error in message size test: " << e.what() << std::endl;
        continue;
      }
    }

    // 不同队列大小的测试
    std::vector<size_t> queue_sizes = {1024, 4096, 8192};  // 减少测试数量
    for (size_t queue_size : queue_sizes) {
      try {
        TestConfig config = base_config;
        config.test_name  = "throughput_queue_" + std::to_string(queue_size);
        config.queue_size = queue_size;

        PerformanceTest test(config);
        PerfResult result     = test.run_throughput_test();
        result.memory_used_kb = test.get_memory_usage();

        test.print_results(result,
            "Throughput Test (Queue Size: " + std::to_string(queue_size) + ")");

        if (config.output_csv) {
          test.write_csv_results(result, "throughput");
        }
      } catch (const std::exception& e) {
        std::cerr << "Error in queue size test: " << e.what() << std::endl;
        continue;
      }
    }

    // 不同工作线程数的测试
    std::vector<size_t> worker_threads = {1, 2, 4};  // 减少测试数量
    for (size_t threads : worker_threads) {
      try {
        TestConfig config     = base_config;
        config.test_name      = "throughput_workers_" + std::to_string(threads);
        config.worker_threads = threads;

        PerformanceTest test(config);
        PerfResult result     = test.run_throughput_test();
        result.memory_used_kb = test.get_memory_usage();

        test.print_results(result, "Throughput Test (Worker Threads: " +
                                       std::to_string(threads) + ")");

        if (config.output_csv) {
          test.write_csv_results(result, "throughput");
        }
      } catch (const std::exception& e) {
        std::cerr << "Error in worker threads test: " << e.what() << std::endl;
        continue;
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Error in throughput test suite: " << e.what() << std::endl;
    throw;  // 重新抛出异常，让调用者处理
  }
}

// 运行延迟测试套件
void run_latency_test_suite(TestConfig base_config) {
  try {
    // 基本配置设置
    base_config.test_name = "latency_suite";

    // 延迟测试使用较少的迭代次数以便更准确地测量
    base_config.iterations        = 5000;  // 减少迭代次数
    base_config.warmup_iterations = 1000;

    std::cout << "Running latency test suite..." << std::endl;

    // 不同线程数的测试
    std::vector<int> thread_counts = {1, 2, 4, 8};  // 减少线程数
    for (int threads : thread_counts) {
      try {
        TestConfig config  = base_config;
        config.test_name   = "latency_threads_" + std::to_string(threads);
        config.num_threads = threads;

        PerformanceTest test(config);
        PerfResult result     = test.run_latency_test();
        result.memory_used_kb = test.get_memory_usage();

        test.print_results(
            result, "Latency Test (" + std::to_string(threads) + " threads)");

        if (config.output_csv) {
          test.write_csv_results(result, "latency");
        }
      } catch (const std::exception& e) {
        std::cerr << "Error in latency thread test: " << e.what() << std::endl;
        continue;
      }
    }

    // 不同消息大小的测试
    std::vector<MessageSize> message_sizes = {
        MessageSize::SMALL, MessageSize::MEDIUM};  // 移除LARGE以减少内存使用
    for (MessageSize size : message_sizes) {
      try {
        TestConfig config = base_config;

        std::string size_name;
        switch (size) {
          case MessageSize::SMALL: size_name = "small"; break;
          case MessageSize::MEDIUM: size_name = "medium"; break;
          case MessageSize::LARGE: size_name = "large"; break;
        }

        config.test_name    = "latency_msgsize_" + size_name;
        config.message_size = size;

        PerformanceTest test(config);
        PerfResult result     = test.run_latency_test();
        result.memory_used_kb = test.get_memory_usage();

        test.print_results(result, "Latency Test (" + size_name + " messages)");

        if (config.output_csv) {
          test.write_csv_results(result, "latency");
        }
      } catch (const std::exception& e) {
        std::cerr << "Error in latency message size test: " << e.what()
                  << std::endl;
        continue;
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Error in latency test suite: " << e.what() << std::endl;
  }
}

// 运行压力测试套件
void run_stress_test_suite(TestConfig base_config) {
  try {
    // 基本配置设置
    base_config.test_name = "stress_suite";

    std::cout << "Running stress test suite..." << std::endl;

    // 不同突发模式的配置
    struct BurstConfig {
      int burst_size;
      int burst_count;
      std::string name;
    };

    std::vector<BurstConfig> burst_configs = {
        {100, 50, "small_bursts"},  // 减少测试量
        {500, 5, "medium_bursts"}   // 减少测试量
    };

    for (const auto& burst_config : burst_configs) {
      try {
        TestConfig config = base_config;
        config.test_name  = "stress_" + burst_config.name;

        PerformanceTest test(config);
        PerfResult result = test.run_stress_test(
            burst_config.burst_size, burst_config.burst_count);
        result.memory_used_kb = test.get_memory_usage();

        test.print_results(result, "Stress Test (" + burst_config.name + ")");

        if (config.output_csv) {
          test.write_csv_results(result, "stress");
        }
      } catch (const std::exception& e) {
        std::cerr << "Error in stress test: " << e.what() << std::endl;
        continue;
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Error in stress test suite: " << e.what() << std::endl;
  }
}

// 运行比较测试套件（mm_logger vs spdlog）
void run_comparison_test_suite(TestConfig base_config) {
  try {
    std::cout << "Running comparison test suite (mm_logger vs spdlog)..."
              << std::endl;

    // 首先使用mm_logger运行
    {
      TestConfig config    = base_config;
      config.test_name     = "compare_mm_logger";
      config.use_mm_logger = true;

      try {
        PerformanceTest test(config);
        PerfResult throughput_result     = test.run_throughput_test();
        throughput_result.memory_used_kb = test.get_memory_usage();

        test.print_results(throughput_result, "MM-Logger Throughput");

        if (config.output_csv) {
          test.write_csv_results(throughput_result, "comparison_throughput");
        }

        PerfResult latency_result     = test.run_latency_test();
        latency_result.memory_used_kb = test.get_memory_usage();

        test.print_results(latency_result, "MM-Logger Latency");

        if (config.output_csv) {
          test.write_csv_results(latency_result, "comparison_latency");
        }
      } catch (const std::exception& e) {
        std::cerr << "Error in mm_logger comparison test: " << e.what()
                  << std::endl;
      }
    }

    // 然后使用原生spdlog运行
    {
      TestConfig config    = base_config;
      config.test_name     = "compare_spdlog";
      config.use_mm_logger = false;

      try {
        PerformanceTest test(config);
        PerfResult throughput_result     = test.run_throughput_test();
        throughput_result.memory_used_kb = test.get_memory_usage();

        test.print_results(throughput_result, "Spdlog Throughput");

        if (config.output_csv) {
          test.write_csv_results(throughput_result, "comparison_throughput");
        }

        PerfResult latency_result     = test.run_latency_test();
        latency_result.memory_used_kb = test.get_memory_usage();

        test.print_results(latency_result, "Spdlog Latency");

        if (config.output_csv) {
          test.write_csv_results(latency_result, "comparison_latency");
        }
      } catch (const std::exception& e) {
        std::cerr << "Error in spdlog comparison test: " << e.what()
                  << std::endl;
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Error in comparison test suite: " << e.what() << std::endl;
  }
}

// test_suites.cpp 中 run_single_test 函数的改进部分

bool run_single_test(TestConfig& config) {
  try {
    // 确保日志目录存在
    if (!utils::create_directory(config.log_dir)) {
      std::cerr << "Error: Failed to create log directory: " << config.log_dir
                << std::endl;
      return false;
    }

    // 确保测试名称不为空
    if (config.test_name.empty()) {
      std::cerr << "Error: Empty test name specified" << std::endl;
      return false;
    }

    std::cout << "Running test: " << config.test_name << std::endl;

    // 打印更多的配置信息，帮助诊断问题
    if (config.verbose) {
      std::cout << "Test configuration:" << std::endl;
      std::cout << "  Message size: "
                << get_message_size_name(config.message_size) << std::endl;
      std::cout << "  Threads: " << config.num_threads << std::endl;
      std::cout << "  Iterations: " << config.iterations << std::endl;
      std::cout << "  Queue size: " << config.queue_size << std::endl;
      std::cout << "  Worker threads: " << config.worker_threads << std::endl;
    }

    // 提取测试的基本类型（删除可能的后缀）
    std::string base_test_name = config.test_name;
    size_t pos                 = base_test_name.find("_msgsize_");
    if (pos != std::string::npos) {
      base_test_name = base_test_name.substr(0, pos);
      std::cout << "Extracted base test name: " << base_test_name << std::endl;
    }

    // 基于测试的基本类型运行测试
    if (config.test_name == "throughput" || base_test_name == "throughput") {
      // 运行单个吞吐量测试
      PerformanceTest test(config);
      PerfResult result     = test.run_throughput_test();
      result.memory_used_kb = test.get_memory_usage();

      test.print_results(result, "Throughput Test");

      if (config.output_csv) {
        test.write_csv_results(result, "throughput");
      }
      return true;
    } else if (config.test_name == "latency" || base_test_name == "latency") {
      // 运行单个延迟测试
      PerformanceTest test(config);
      PerfResult result     = test.run_latency_test();
      result.memory_used_kb = test.get_memory_usage();

      test.print_results(result, "Latency Test");

      if (config.output_csv) {
        test.write_csv_results(result, "latency");
      }
      return true;
    } else if (config.test_name == "stress" || base_test_name == "stress") {
      // 运行单个压力测试，默认5组500条日志的突发
      PerformanceTest test(config);
      PerfResult result = test.run_stress_test(500, 5);  // 减少测试规模
      result.memory_used_kb = test.get_memory_usage();

      test.print_results(result, "Stress Test");

      if (config.output_csv) {
        test.write_csv_results(result, "stress");
      }
      return true;
    } else if (config.test_name == "compare" || base_test_name == "compare") {
      try {
        run_comparison_test_suite(config);
        return true;
      } catch (const std::exception& e) {
        std::cerr << "Error in comparison test: " << e.what() << std::endl;
        return false;
      }
    }
    // 其他测试类型保持不变...

    std::cerr << "Unknown test name: '" << config.test_name << "'" << std::endl;
    std::cerr << "Available tests: throughput, latency, stress, compare, "
                 "throughput_suite, latency_suite, stress_suite, all"
              << std::endl;
    return false;
  } catch (const std::exception& e) {
    // 打印详细错误信息
    std::cerr << "Error type: " << typeid(e).name() << std::endl;
    std::cerr << "Error during test execution: " << e.what() << std::endl;
    return false;
  } catch (...) {
    std::cerr << "Unknown exception during test execution" << std::endl;
    return false;
  }
}
