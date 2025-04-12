/**
 * Performance benchmark tool for mm_logger
 * 
 * This program tests various performance aspects of the mm_logger library:
 * - Maximum throughput (logs per second)
 * - Latency of logging operations
 * - Performance under different configurations
 * - Memory usage patterns
 * - Disk I/O performance
 * - Comparison with native spdlog
 * 
 * Usage: performance_test [options]
 */

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
#include <condition_variable>
#include <memory>
#include <cstring>
#include <functional>
#include <random>
#include <filesystem>

// Direct spdlog usage for comparison
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"

// Barrier to synchronize thread startup
class ThreadBarrier {
public:
    explicit ThreadBarrier(size_t count) 
        : threshold_(count), count_(count), generation_(0) {

// Run a series of latency tests with different configurations
void run_latency_test_suite(TestConfig base_config) {
    // Base configuration for latency test
    base_config.test_name = "latency";
    
    // For latency tests, we use fewer iterations to allow for more accurate measurements
    base_config.iterations = 10000;
    base_config.warmup_iterations = 1000;
    
    std::cout << "Running latency test suite..." << std::endl;
    
    // Test with varying thread counts
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
    
    // Test with varying message sizes
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

// Run stress tests with various burst patterns
void run_stress_test_suite(TestConfig base_config) {
    // Base configuration for stress test
    base_config.test_name = "stress";
    
    std::cout << "Running stress test suite..." << std::endl;
    
    // Array of burst sizes and counts to test
    struct BurstConfig {
        int burst_size;
        int burst_count;
        std::string name;
    };
    
    std::vector<BurstConfig> burst_configs = {
        {100, 100, "small_bursts"},     // 100 bursts of 100 logs each
        {1000, 10, "medium_bursts"},    // 10 bursts of 1000 logs each
        {10000, 1, "large_burst"}       // 1 burst of 10000 logs
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

// Compare mm_logger with direct spdlog
void run_comparison_test_suite(TestConfig base_config) {
    std::cout << "Running comparison test suite (mm_logger vs spdlog)..." << std::endl;
    
    // First run with mm_logger
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
    
    // Then run with direct spdlog
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

int main(int argc, char* argv[]) {
    // Parse command line arguments
    TestConfig config = parse_args(argc, argv);
    
    // Create log directory
    std::filesystem::create_directories(config.log_dir);
    
    if (config.test_name == "throughput") {
        // Run a single throughput test
        PerformanceTest test(config);
        PerfResult result = test.run_throughput_test();
        result.memory_used_kb = test.get_memory_usage();
        
        test.print_results(result, "Throughput Test");
        
        if (config.output_csv) {
            test.write_csv_results(result, "throughput");
        }
    } else if (config.test_name == "latency") {
        // Run a single latency test
        PerformanceTest test(config);
        PerfResult result = test.run_latency_test();
        result.memory_used_kb = test.get_memory_usage();
        
        test.print_results(result, "Latency Test");
        
        if (config.output_csv) {
            test.write_csv_results(result, "latency");
        }
    } else if (config.test_name == "stress") {
        // Run a stress test with a single burst pattern
        PerformanceTest test(config);
        PerfResult result = test.run_stress_test(1000, 10); // Default: 10 bursts of 1000 logs
        result.memory_used_kb = test.get_memory_usage();
        
        test.print_results(result, "Stress Test");
        
        if (config.output_csv) {
            test.write_csv_results(result, "stress");
        }
    } else if (config.test_name == "compare") {
        run_comparison_test_suite(config);
    } else if (config.test_name == "throughput_suite") {
        run_throughput_test_suite(config);
    } else if (config.test_name == "latency_suite") {
        run_latency_test_suite(config);
    } else if (config.test_name == "stress_suite") {
        run_stress_test_suite(config);
    } else if (config.test_name == "all") {
        // Run all test suites
        run_throughput_test_suite(config);
        run_latency_test_suite(config);
        run_stress_test_suite(config);
        run_comparison_test_suite(config);
    } else {
        std::cerr << "Unknown test: " << config.test_name << std::endl;
        print_help();
        return 1;
    }
    
    std::cout << "All tests completed." << std::endl;
    if (config.output_csv) {
        std::cout << "Results written to " << config.csv_file << std::endl;
    }
    
    return 0;
}
}

    void wait() {
        std::unique_lock<std::mutex> lock(mutex_);
        auto generation = generation_;
        if (--count_ == 0) {
            count_ = threshold_;
            ++generation_;
            cv_.notify_all();
        } else {
            cv_.wait(lock, [this, generation] { return generation != generation_; });
        }
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    size_t threshold_;
    size_t count_;
    size_t generation_;
};

// Log message sizes
enum class MessageSize {
    SMALL,   // ~64 bytes
    MEDIUM,  // ~256 bytes
    LARGE    // ~1024 bytes
};

// Test configuration
struct TestConfig {
    // General settings
    std::string test_name;
    std::string log_dir = "./perf_logs";
    std::string log_prefix;
    int num_threads = 8;
    int iterations = 100000;
    int warmup_iterations = 10000;
    bool use_mm_logger = true;
    
    // Message settings
    MessageSize message_size = MessageSize::MEDIUM;
    bool randomize_message = false;
    
    // Logger settings
    size_t max_file_size_mb = 10;
    size_t max_total_size_mb = 100;
    bool enable_debug = true;
    bool enable_console = false;
    bool enable_file = true;
    size_t queue_size = 8192;
    size_t worker_threads = 2;
    
    // Output settings
    bool output_csv = false;
    std::string csv_file = "performance_results.csv";
    bool verbose = false;
};

// Performance results
struct PerfResult {
    double total_time_ms = 0;
    double logs_per_second = 0;
    std::vector<double> latencies_us;
    double min_latency_us = 0;
    double max_latency_us = 0;
    double median_latency_us = 0;
    double p95_latency_us = 0;
    double p99_latency_us = 0;
    size_t memory_used_kb = 0;
};

// Test cases
class PerformanceTest {
public:
    PerformanceTest(const TestConfig& config) : config_(config) {
        // Create log directory
        std::filesystem::create_directories(config_.log_dir);
        
        // Set log prefix
        config_.log_prefix = config_.log_dir + "/" + config_.test_name;
        
        // Initialize logger
        if (config_.use_mm_logger) {
            init_mm_logger();
        } else {
            init_spdlog();
        }
        
        // Pre-generate test messages
        generate_test_messages();
    }
    
    ~PerformanceTest() {
        if (config_.use_mm_logger) {
            mm_log::Logger::Instance().Shutdown();
        } else {
            spdlog::shutdown();
        }
    }
    
    PerfResult run_throughput_test() {
        PerfResult result;
        std::atomic<int> completed_logs(0);
        std::vector<std::thread> threads;
        ThreadBarrier barrier(config_.num_threads);
        
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
    
    PerfResult run_latency_test() {
        PerfResult result;
        std::vector<double> latencies;
        std::mutex latencies_mutex;
        std::vector<std::thread> threads;
        ThreadBarrier barrier(config_.num_threads);
        
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
    
    PerfResult run_stress_test(int burst_size, int burst_count) {
        PerfResult result;
        std::atomic<int> completed_logs(0);
        std::vector<std::thread> threads;
        ThreadBarrier barrier(config_.num_threads);
        
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
    
    // Get memory usage in KB
    size_t get_memory_usage() {
        // Note: This is a Linux-specific implementation
        // For a real cross-platform solution, use a library like PAPI or custom code
        size_t memory_kb = 0;
        std::ifstream status_file("/proc/self/status");
        
        if (status_file.is_open()) {
            std::string line;
            while (std::getline(status_file, line)) {
                if (line.find("VmRSS:") != std::string::npos) {
                    std::stringstream ss(line);
                    std::string name;
                    ss >> name >> memory_kb;
                    break;
                }
            }
            status_file.close();
        }
        
        return memory_kb;
    }
    
    void print_results(const PerfResult& result, const std::string& test_type) {
        std::cout << "========================= " << test_type << " Results =========================" << std::endl;
        std::cout << "Test Name: " << config_.test_name << std::endl;
        std::cout << "Total Logs: " << (config_.num_threads * config_.iterations) << std::endl;
        std::cout << "Threads: " << config_.num_threads << std::endl;
        std::cout << "Total Time: " << std::fixed << std::setprecision(2) << result.total_time_ms << " ms" << std::endl;
        std::cout << "Logs Per Second: " << std::fixed << std::setprecision(2) << result.logs_per_second << std::endl;
        
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
    
    void write_csv_results(const PerfResult& result, const std::string& test_type) {
        bool file_exists = std::filesystem::exists(config_.csv_file);
        std::ofstream csv_file(config_.csv_file, std::ios::app);
        
        if (!file_exists) {
            // Write header
            csv_file << "Timestamp,TestName,TestType,Logger,Threads,Iterations,MessageSize,QueueSize,WorkerThreads,EnableConsole,EnableFile,"
                     << "TotalTime_ms,LogsPerSecond,Min_Latency_us,Median_Latency_us,P95_Latency_us,P99_Latency_us,Max_Latency_us,Memory_KB"
                     << std::endl;
        }
        
        // Get current timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        char timestamp[64];
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&time_t_now));
        
        // Write data row
        csv_file << timestamp << ","
                 << config_.test_name << ","
                 << test_type << ","
                 << (config_.use_mm_logger ? "mm_logger" : "spdlog") << ","
                 << config_.num_threads << ","
                 << config_.iterations << ","
                 << get_message_size_name() << ","
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
    }
    
private:
    // Initialize mm_logger
    void init_mm_logger() {
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
    
    // Initialize spdlog
    void init_spdlog() {
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
    
    // Generate test messages of different sizes
    void generate_test_messages() {
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
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, 35); // 0-9, A-Z
        
        char char_set[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        std::string payload(payload_size, ' ');
        
        for (size_t i = 0; i < payload_size; ++i) {
            payload[i] = char_set[dist(gen)];
        }
        
        test_message_ = base_message + payload;
    }
    
    // Log a message
    void log_message(int thread_id, int iteration, bool flush) {
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
        
        if (flush) {
            if (config_.use_mm_logger) {
                // There's no explicit flush in mm_logger API, so we rely on async flushing
            } else {
                spdlog::flush();
            }
        }
    }
    
    std::string get_message_size_name() const {
        switch (config_.message_size) {
            case MessageSize::SMALL: return "Small";
            case MessageSize::MEDIUM: return "Medium";
            case MessageSize::LARGE: return "Large";
            default: return "Unknown";
        }
    }
    
    TestConfig config_;
    std::string test_message_;
};

// Print command-line help
void print_help() {
    std::cout << "MM-Logger Performance Test Tool" << std::endl;
    std::cout << "Usage: performance_test [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --help               Show this help message" << std::endl;
    std::cout << "  --test=NAME          Test name (throughput, latency, stress, compare)" << std::endl;
    std::cout << "  --threads=N          Number of threads (default: 8)" << std::endl;
    std::cout << "  --iterations=N       Iterations per thread (default: 100000)" << std::endl;
    std::cout << "  --warmup=N           Warmup iterations (default: 10000)" << std::endl;
    std::cout << "  --use-mm-logger      Use mm_logger (default)" << std::endl;
    std::cout << "  --use-spdlog         Use spdlog directly for comparison" << std::endl;
    std::cout << "  --message-size=SIZE  Message size (small, medium, large; default: medium)" << std::endl;
    std::cout << "  --max-file-size=N    Max file size in MB (default: 10)" << std::endl;
    std::cout << "  --max-total-size=N   Max total size in MB (default: 100)" << std::endl;
    std::cout << "  --queue-size=N       Async queue size (default: 8192)" << std::endl;
    std::cout << "  --worker-threads=N   Worker threads (default: 2)" << std::endl;
    std::cout << "  --enable-console     Enable console output (default: disabled for performance)" << std::endl;
    std::cout << "  --disable-file       Disable file output" << std::endl;
    std::cout << "  --csv=FILE           Output CSV results to file" << std::endl;
    std::cout << "  --verbose            Enable verbose output" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  ./performance_test --test=throughput --threads=16 --iterations=50000" << std::endl;
    std::cout << "  ./performance_test --test=latency --threads=4 --iterations=10000 --message-size=small" << std::endl;
    std::cout << "  ./performance_test --test=compare --threads=8 --csv=results.csv" << std::endl;
}

// Parse command-line arguments
TestConfig parse_args(int argc, char* argv[]) {
    TestConfig config;
    config.test_name = "throughput"; // Default test
    
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
            } else if (size == "medium") {
                config.message_size = MessageSize::MEDIUM;
            } else if (size == "large") {
                config.message_size = MessageSize::LARGE;
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
            config.csv_file = arg.substr(6);
        } else if (arg == "--verbose") {
            config.verbose = true;
        }
    }
    
    return config;
}

// Run a series of throughput tests with different configurations
void run_throughput_test_suite(TestConfig base_config) {
    // Base configuration for throughput test
    base_config.test_name = "throughput";
    
    std::cout << "Running throughput test suite..." << std::endl;
    
    // Test with varying thread counts
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
    
    // Test with varying message sizes
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
    
    // Test with varying queue sizes
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
    
    // Test with varying worker thread counts
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
