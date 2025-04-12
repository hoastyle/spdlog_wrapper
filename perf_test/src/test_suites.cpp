/**
 * @file test_suites.cpp
 * @brief 测试套件函数实现
 */

#include "test_suites.h"
#include "perf_test.h"
#include <iostream>
#include <vector>

// 运行吞吐量测试套件
void run_throughput_test_suite(TestConfig base_config) {
    // 基本配置设置
    base_config.test_name = "throughput_suite";
    
    std::cout << "Running throughput test suite..." << std::endl;
    
    // 不同线程数的测试
    std::vector<int> thread_counts = {1, 2, 4, 8, 16, 32, 64};
    for (int threads : thread_counts) {
        TestConfig config = base_config;
        config.test_name = "throughput_threads_" + std::to_string(threads);
        config.num_threads = threads;
        
        PerformanceTest test(config);
        PerfResult result = test.run_throughput_test();
        result.memory_used_kb = test.get_memory_usage();
        
        test.print_results(result, "Throughput Test (" + std::to_string(threads) + " threads)");
        
        if (config.output_csv) {
            test.write_csv_results(result, "throughput");
        }
    }
    
    // 不同消息大小的测试
    std::vector<MessageSize> message_sizes = {MessageSize::SMALL, MessageSize::MEDIUM, MessageSize::LARGE};
    for (MessageSize size : message_sizes) {
        TestConfig config = base_config;
        
        std::string size_name;
        switch (size) {
            case MessageSize::SMALL: size_name = "small"; break;
            case MessageSize::MEDIUM: size_name = "medium"; break;
            case MessageSize::LARGE: size_name = "large"; break;
        }
        
        config.test_name = "throughput_msgsize_" + size_name;
        config.message_size = size;
        
        PerformanceTest test(config);
        PerfResult result = test.run_throughput_test();
        result.memory_used_kb = test.get_memory_usage();
        
        test.print_results(result, "Throughput Test (" + size_name + " messages)");
        
        if (config.output_csv) {
            test.write_csv_results(result, "throughput");
        }
    }
    
    // 不同队列大小的测试
    std::vector<size_t> queue_sizes = {1024, 4096, 8192, 16384, 32768};
    for (size_t queue_size : queue_sizes) {
        TestConfig config = base_config;
        config.test_name = "throughput_queue_" + std::to_string(queue_size);
        config.queue_size = queue_size;
        
        PerformanceTest test(config);
        PerfResult result = test.run_throughput_test();
        result.memory_used_kb = test.get_memory_usage();
        
        test.print_results(result, "Throughput Test (Queue Size: " + std::to_string(queue_size) + ")");
        
        if (config.output_csv) {
            test.write_csv_results(result, "throughput");
        }
    }
    
    // 不同工作线程数的测试
    std::vector<size_t> worker_threads = {1, 2, 4, 8};
    for (size_t threads : worker_threads) {
        TestConfig config = base_config;
        config.test_name = "throughput_workers_" + std::to_string(threads);
        config.worker_threads = threads;
        
        PerformanceTest test(config);
        PerfResult result = test.run_throughput_test();
        result.memory_used_kb = test.get_memory_usage();
        
        test.print_results(result, "Throughput Test (Worker Threads: " + std::to_string(threads) + ")");
        
        if (config.output_csv) {
            test.write_csv_results(result, "throughput");
        }
    }
}

// 运行延迟测试套件
void run_latency_test_suite(TestConfig base_config) {
    // 基本配置设置
    base_config.test_name = "latency_suite";
    
    // 延迟测试使用较少的迭代次数以便更准确地测量
    base_config.iterations = 10000;
    base_config.warmup_iterations = 1000;
    
    std::cout << "Running latency test suite..." << std::endl;
    
    // 不同线程数的测试
    std::vector<int> thread_counts = {1, 2, 4, 8, 16};
    for (int threads : thread_counts) {
        TestConfig config = base_config;
        config.test_name = "latency_threads_" + std::to_string(threads);
        config.num_threads = threads;
        
        PerformanceTest test(config);
        PerfResult result = test.run_latency_test();
        result.memory_used_kb = test.get_memory_usage();
        
        test.print_results(result, "Latency Test (" + std::to_string(threads) + " threads)");
        
        if (config.output_csv) {
            test.write_csv_results(result, "latency");
        }
    }
    
    // 不同消息大小的测试
    std::vector<MessageSize> message_sizes = {MessageSize::SMALL, MessageSize::MEDIUM, MessageSize::LARGE};
    for (MessageSize size : message_sizes) {
        TestConfig config = base_config;
        
        std::string size_name;
        switch (size) {
            case MessageSize::SMALL: size_name = "small"; break;
            case MessageSize::MEDIUM: size_name = "medium"; break;
            case MessageSize::LARGE: size_name = "large"; break;
        }
        
        config.test_name = "latency_msgsize_" + size_name;
        config.message_size = size;
        
        PerformanceTest test(config);
        PerfResult result = test.run_latency_test();
        result.memory_used_kb = test.get_memory_usage();
        
        test.print_results(result, "Latency Test (" + size_name + " messages)");
        
        if (config.output_csv) {
            test.write_csv_results(result, "latency");
        }
    }
}

// 运行压力测试套件
void run_stress_test_suite(TestConfig base_config) {
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
        {100, 100, "small_bursts"},     // 100组100条日志的突发
        {1000, 10, "medium_bursts"},    // 10组1000条日志的突发
        {10000, 1, "large_burst"}       // 1组10000条日志的突发
    };
    
    for (const auto& burst_config : burst_configs) {
        TestConfig config = base_config;
        config.test_name = "stress_" + burst_config.name;
        
        PerformanceTest test(config);
        PerfResult result = test.run_stress_test(burst_config.burst_size, burst_config.burst_count);
        result.memory_used_kb = test.get_memory_usage();
        
        test.print_results(result, "Stress Test (" + burst_config.name + ")");
        
        if (config.output_csv) {
            test.write_csv_results(result, "stress");
        }
    }
}

// 运行比较测试套件（mm_logger vs spdlog）
void run_comparison_test_suite(TestConfig base_config) {
    std::cout << "Running comparison test suite (mm_logger vs spdlog)..." << std::endl;
    
    // 首先使用mm_logger运行
    {
        TestConfig config = base_config;
        config.test_name = "compare_mm_logger";
        config.use_mm_logger = true;
        
        PerformanceTest test(config);
        PerfResult throughput_result = test.run_throughput_test();
        throughput_result.memory_used_kb = test.get_memory_usage();
        
        test.print_results(throughput_result, "MM-Logger Throughput");
        
        if (config.output_csv) {
            test.write_csv_results(throughput_result, "comparison_throughput");
        }
        
        PerfResult latency_result = test.run_latency_test();
        latency_result.memory_used_kb = test.get_memory_usage();
        
        test.print_results(latency_result, "MM-Logger Latency");
        
        if (config.output_csv) {
            test.write_csv_results(latency_result, "comparison_latency");
        }
    }
    
    // 然后使用原生spdlog运行
    {
        TestConfig config = base_config;
        config.test_name = "compare_spdlog";
        config.use_mm_logger = false;
        
        PerformanceTest test(config);
        PerfResult throughput_result = test.run_throughput_test();
        throughput_result.memory_used_kb = test.get_memory_usage();
        
        test.print_results(throughput_result, "Spdlog Throughput");
        
        if (config.output_csv) {
            test.write_csv_results(throughput_result, "comparison_throughput");
        }
        
        PerfResult latency_result = test.run_latency_test();
        latency_result.memory_used_kb = test.get_memory_usage();
        
        test.print_results(latency_result, "Spdlog Latency");
        
        if (config.output_csv) {
            test.write_csv_results(latency_result, "comparison_latency");
        }
    }
}

// 运行单个测试
bool run_single_test(TestConfig& config) {
    try {
        if (config.test_name == "throughput") {
            // 运行单个吞吐量测试
            PerformanceTest test(config);
            PerfResult result = test.run_throughput_test();
            result.memory_used_kb = test.get_memory_usage();
            
            test.print_results(result, "Throughput Test");
            
            if (config.output_csv) {
                test.write_csv_results(result, "throughput");
            }
            return true;
        } else if (config.test_name == "latency") {
            // 运行单个延迟测试
            PerformanceTest test(config);
            PerfResult result = test.run_latency_test();
            result.memory_used_kb = test.get_memory_usage();
            
            test.print_results(result, "Latency Test");
            
            if (config.output_csv) {
                test.write_csv_results(result, "latency");
            }
            return true;
        } else if (config.test_name == "stress") {
            // 运行单个压力测试，默认10组1000条日志的突发
            PerformanceTest test(config);
            PerfResult result = test.run_stress_test(1000, 10);
            result.memory_used_kb = test.get_memory_usage();
            
            test.print_results(result, "Stress Test");
            
            if (config.output_csv) {
                test.write_csv_results(result, "stress");
            }
            return true;
        } else if (config.test_name == "compare") {
            run_comparison_test_suite(config);
            return true;
        } else if (config.test_name == "throughput_suite") {
            run_throughput_test_suite(config);
            return true;
        } else if (config.test_name == "latency_suite") {
            run_latency_test_suite(config);
            return true;
        } else if (config.test_name == "stress_suite") {
            run_stress_test_suite(config);
            return true;
        } else if (config.test_name == "all") {
            // 运行所有测试套件
            run_throughput_test_suite(config);
            run_latency_test_suite(config);
            run_stress_test_suite(config);
            run_comparison_test_suite(config);
            return true;
        }
        
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Error during test execution: " << e.what() << std::endl;
        return false;
    }
}
