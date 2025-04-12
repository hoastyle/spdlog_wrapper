/**
 * @file perf_test.cpp
 * @brief 性能测试类实现
 */

#include "perf_test.h"
#include "utils.h"
#include "mm_logger/mm_logger_all.hpp"

#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <mutex>
#include <functional>
#include <random>
#include <filesystem>
#include <iostream>

// Direct spdlog usage for comparison
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"

// Thread barrier implementation
ThreadBarrier::ThreadBarrier(size_t count)
    : threshold_(count), count_(count), generation_(0) {}

void ThreadBarrier::wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    auto generation = generation_;
    if (--count_ == 0) {
        // Last thread to reach the barrier
        count_ = threshold_;
        ++generation_;
        cv_.notify_all();
    } else {
        // Wait for the last thread
        cv_.wait(lock, [this, generation] { return generation != generation_; });
    }
}

// PerformanceTest implementation
PerformanceTest::PerformanceTest(const TestConfig& config) 
    : config_(config) {
    
    // Create log directory
    utils::create_directory(config_.log_dir);
    
    // Set log prefix with full path
    config_.log_prefix = config_.log_dir + "/" + config_.test_name;
    
    // Initialize logger
    if (config_.use_mm_logger) {
        init_mm_logger();
    } else {
        init_spdlog();
    }
    
    // Generate test messages
    generate_test_messages();
}

PerformanceTest::~PerformanceTest() {
    if (config_.use_mm_logger) {
        mm_log::Logger::Instance().Shutdown();
    } else {
        spdlog::shutdown();
    }
}

void PerformanceTest::init_mm_logger() {
    mm_log::Logger::Instance().Initialize(
        config_.log_prefix,
        config_.max_file_size_mb,
        config_.max_total_size_mb,
        config_.enable_debug,
        config_.enable_console,
        config_.enable_file,
        config_.queue_size,
        config_.worker_threads
    );
}

void PerformanceTest::init_spdlog() {
    // Initialize spdlog with similar settings to mm_logger for comparison
    spdlog::init_thread_pool(config_.queue_size, config_.worker_threads);
    
    std::vector<spdlog::sink_ptr> sinks;
    
    if (config_.enable_console) {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(config_.enable_debug ? spdlog::level::debug : spdlog::level::info);
        sinks.push_back(console_sink);
    }
    
    if (config_.enable_file) {
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            config_.log_prefix + ".log", true);
        file_sink->set_level(config_.enable_debug ? spdlog::level::debug : spdlog::level::info);
        sinks.push_back(file_sink);
    }
    
    auto logger = std::make_shared<spdlog::async_logger>(
        "spdlog_logger",
        sinks.begin(),
        sinks.end(),
        spdlog::thread_pool(),
        spdlog::async_overflow_policy::block
    );
    
    logger->set_level(config_.enable_debug ? spdlog::level::debug : spdlog::level::info);
    spdlog::register_logger(logger);
    spdlog::set_default_logger(logger);
}

void PerformanceTest::generate_test_messages() {
    size_t target_size = 0;
    
    switch (config_.message_size) {
        case MessageSize::SMALL:
            target_size = 64;
            break;
        case MessageSize::MEDIUM:
            target_size = 256;
            break;
        case MessageSize::LARGE:
            target_size = 1024;
            break;
    }
    
    // Base message template
    std::string base_message = "Performance test message from thread %d, iteration %d, with payload: ";
    
    // Calculate payload size to reach target_size
    size_t payload_size = target_size - base_message.size() - 20; // 20 for numbers and formatting
    
    // Generate random payload
    test_message_ = base_message + utils::generate_random_string(payload_size);
}

void PerformanceTest::log_message(int thread_id, int iteration, bool flush) {
    if (config_.use_mm_logger) {
        switch (thread_id % 4) {
            case 0:
                MM_DEBUG(test_message_.c_str(), thread_id, iteration);
                break;
            case 1:
                MM_INFO(test_message_.c_str(), thread_id, iteration);
                break;
            case 2:
                MM_WARN(test_message_.c_str(), thread_id, iteration);
                break;
            case 3:
                MM_ERROR(test_message_.c_str(), thread_id, iteration);
                break;
        }
    } else {
        switch (thread_id % 4) {
            case 0:
                spdlog::debug(test_message_.c_str(), thread_id, iteration);
                break;
            case 1:
                spdlog::info(test_message_.c_str(), thread_id, iteration);
                break;
            case 2:
                spdlog::warn(test_message_.c_str(), thread_id, iteration);
                break;
            case 3:
                spdlog::error(test_message_.c_str(), thread_id, iteration);
                break;
        }
    }
    
    // Flush is a no-op for now since we don't have a direct flush API in mm_logger
    // and spdlog::flush() is deprecated
    (void)flush;
}

PerfResult PerformanceTest::run_throughput_test() {
    PerfResult result;
    std::atomic<int> completed_logs(0);
    std::vector<std::thread> threads;
    ThreadBarrier barrier(config_.num_threads);
    
    if (config_.verbose) {
        std::cout << "Starting throughput test with " << config_.num_threads << " threads, "
                << config_.iterations << " iterations per thread" << std::endl;
    }
    
    // Start throughput measurement
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Launch worker threads
    for (int i = 0; i < config_.num_threads; ++i) {
        threads.emplace_back([this, i, &barrier, &completed_logs]() {
            // Wait for all threads to be ready
            barrier.wait();
            
            // Run warmup iterations
            for (int j = 0; j < config_.warmup_iterations; ++j) {
                log_message(i, j, false);
            }
            
            // Run actual test iterations
            for (int j = 0; j < config_.iterations; ++j) {
                log_message(i, j, false);
                completed_logs.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }
    
    // Calculate elapsed time and throughput
    auto end_time = std::chrono::high_resolution_clock::now();
    result.total_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    result.logs_per_second = (completed_logs.load() * 1000.0) / result.total_time_ms;
    
    return result;
}

PerfResult PerformanceTest::run_latency_test() {
    PerfResult result;
    std::vector<double> latencies;
    std::mutex latencies_mutex;
    std::vector<std::thread> threads;
    ThreadBarrier barrier(config_.num_threads);
    
    if (config_.verbose) {
        std::cout << "Starting latency test with " << config_.num_threads << " threads, "
                << config_.iterations << " iterations per thread" << std::endl;
    }
    
    // Launch worker threads
    for (int i = 0; i < config_.num_threads; ++i) {
        threads.emplace_back([this, i, &barrier, &latencies, &latencies_mutex]() {
            std::vector<double> thread_latencies;
            thread_latencies.reserve(config_.iterations);
            
            // Wait for all threads to be ready
            barrier.wait();
            
            // Run warmup iterations
            for (int j = 0; j < config_.warmup_iterations; ++j) {
                log_message(i, j, false);
            }
            
            // Run actual test iterations with timing
            for (int j = 0; j < config_.iterations; ++j) {
                auto start = std::chrono::high_resolution_clock::now();
                log_message(i, j, true);
                auto end = std::chrono::high_resolution_clock::now();
                
                double latency = std::chrono::duration<double, std::micro>(end - start).count();
                thread_latencies.push_back(latency);
                
                // Small delay to prevent overwhelming the system
                if (j % 100 == 0) {
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }
            }
            
            // Merge results
            {
                std::lock_guard<std::mutex> lock(latencies_mutex);
                latencies.insert(latencies.end(), thread_latencies.begin(), thread_latencies.end());
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }
    
    // Calculate latency statistics
    if (!latencies.empty()) {
        result.latencies_us = std::move(latencies);
        std::sort(result.latencies_us.begin(), result.latencies_us.end());
        
        result.min_latency_us = result.latencies_us.front();
        result.max_latency_us = result.latencies_us.back();
        
        size_t mid_idx = result.latencies_us.size() / 2;
        result.median_latency_us = result.latencies_us[mid_idx];
        
        size_t p95_idx = result.latencies_us.size() * 0.95;
        size_t p99_idx = result.latencies_us.size() * 0.99;
        
        result.p95_latency_us = result.latencies_us[p95_idx];
        result.p99_latency_us = result.latencies_us[p99_idx];
    }
    
    return result;
}

PerfResult PerformanceTest::run_stress_test(int burst_size, int burst_count) {
    PerfResult result;
    std::atomic<int> completed_logs(0);
    std::vector<std::thread> threads;
    ThreadBarrier barrier(config_.num_threads);
    
    if (config_.verbose) {
        std::cout << "Starting stress test with " << config_.num_threads << " threads, "
                << burst_count << " bursts of " << burst_size << " logs each" << std::endl;
    }
    
    // Start throughput measurement
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Launch worker threads
    for (int i = 0; i < config_.num_threads; ++i) {
        threads.emplace_back([this, i, burst_size, burst_count, &barrier, &completed_logs]() {
            // Wait for all threads to be ready
            barrier.wait();
            
            for (int burst = 0; burst < burst_count; ++burst) {
                // Generate a burst of logs
                for (int j = 0; j < burst_size; ++j) {
                    log_message(i, j + (burst * burst_size), false);
                    completed_logs.fetch_add(1, std::memory_order_relaxed);
                }
                
                // Small delay between bursts
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }
    
    // Calculate elapsed time and throughput
    auto end_time = std::chrono::high_resolution_clock::now();
    result.total_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    result.logs_per_second = (completed_logs.load() * 1000.0) / result.total_time_ms;
    
    return result;
}

size_t PerformanceTest::get_memory_usage() {
    return utils::get_process_memory_usage();
}

void PerformanceTest::print_results(const PerfResult& result, const std::string& test_type) {
    std::cout << "========================= " << test_type << " Results =========================" << std::endl;
    std::cout << "Test Name: " << config_.test_name << std::endl;
    std::cout << "Logger: " << (config_.use_mm_logger ? "mm_logger" : "spdlog") << std::endl;
    std::cout << "Total Logs: " << (config_.num_threads * config_.iterations) << std::endl;
    std::cout << "Threads: " << config_.num_threads << std::endl;
    std::cout << "Message Size: " << get_message_size_name(config_.message_size) << std::endl;
    std::cout << "Queue Size: " << config_.queue_size << std::endl;
    std::cout << "Worker Threads: " << config_.worker_threads << std::endl;
    std::cout << "Console Output: " << (config_.enable_console ? "Enabled" : "Disabled") << std::endl;
    std::cout << "File Output: " << (config_.enable_file ? "Enabled" : "Disabled") << std::endl;
    std::cout << "Total Time: " << std::fixed << std::setprecision(2) << result.total_time_ms << " ms" << std::endl;
    std::cout << "Logs Per Second: " << utils::format_number(result.logs_per_second) << std::endl;
    
    if (!result.latencies_us.empty()) {
        std::cout << "Latency Statistics (µs):" << std::endl;
        std::cout << "  Min: " << std::fixed << std::setprecision(2) << result.min_latency_us << std::endl;
        std::cout << "  Median: " << std::fixed << std::setprecision(2) << result.median_latency_us << std::endl;
        std::cout << "  95th Percentile: " << std::fixed << std::setprecision(2) << result.p95_latency_us << std::endl;
        std::cout << "  99th Percentile: " << std::fixed << std::setprecision(2) << result.p99_latency_us << std::endl;
        std::cout << "  Max: " << std::fixed << std::setprecision(2) << result.max_latency_us << std::endl;
    }
    
    std::cout << "Memory Usage: " << result.memory_used_kb << " KB" << std::endl;
    std::cout << "=======================================================================" << std::endl;
}

void PerformanceTest::write_csv_results(const PerfResult& result, const std::string& test_type) {
    bool file_exists = utils::file_exists(config_.csv_file);
    std::ofstream csv_file(config_.csv_file, std::ios::app);
    
    if (!file_exists) {
        // Write header
        csv_file << "Timestamp,TestName,TestType,Logger,Threads,Iterations,MessageSize,QueueSize,WorkerThreads,EnableConsole,EnableFile,"
                 << "TotalTime_ms,LogsPerSecond,Min_Latency_us,Median_Latency_us,P95_Latency_us,P99_Latency_us,Max_Latency_us,Memory_KB"
                 << std::endl;
    }
    
    // Get current timestamp
    std::string timestamp = utils::get_timestamp_str();
    
    // Write data row
    csv_file << timestamp << ","
             << config_.test_name << ","
             << test_type << ","
             << (config_.use_mm_logger ? "mm_logger" : "spdlog") << ","
             << config_.num_threads << ","
             << config_.iterations << ","
             << get_message_size_name(config_.message_size) << ","
             << config_.queue_size << ","
             << config_.worker_threads << ","
             << (config_.enable_console ? "true" : "false") << ","
             << (config_.enable_file ? "true" : "false") << ","
             << std::fixed << std::setprecision(2) << result.total_time_ms << ","
             << std::fixed << std::setprecision(2) << result.logs_per_second << ","
             << std::fixed << std::setprecision(2) << result.min_latency_us << ","
             << std::fixed << std::setprecision(2) << result.median_latency_us << ","
             << std::fixed << std::setprecision(2) << result.p95_latency_us << ","
             << std::fixed << std::setprecision(2) << result.p99_latency_us << ","
             << std::fixed << std::setprecision(2) << result.max_latency_us << ","
             << result.memory_used_kb
             << std::endl;
             
    if (config_.verbose) {
        std::cout << "Results written to: " << config_.csv_file << std::endl;
    }
}
