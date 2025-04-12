#include "mm_logger/mm_logger.hpp"
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <filesystem>

// Function to generate random string
std::string generate_random_string(size_t length) {
  static const char alphanum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);

  std::string result;
  result.reserve(length);
  for (size_t i = 0; i < length; ++i) {
    result += alphanum[dis(gen)];
  }
  return result;
}

int main(int argc, char* argv[]) {
  // Parse command line arguments
  size_t num_logs       = 10000;       // Default number of logs
  size_t log_size       = 40;          // Default log size (characters)
  size_t max_file_size  = 1024;        // Default single file size (MB)
  size_t max_total_size = 512 * 1024;  // Default total size limit (MB)
  int log_interval_ms   = 0;           // Default no delay

  // Check command line arguments
  if (argc > 1) num_logs = std::stoul(argv[1]);
  if (argc > 2) log_size = std::stoul(argv[2]);
  if (argc > 3) max_file_size = std::stoul(argv[3]);
  if (argc > 4) max_total_size = std::stoul(argv[4]);
  if (argc > 5) log_interval_ms = std::stoi(argv[5]);

  // Initialize log system
  std::string log_dir    = "./logs";
  std::string log_prefix = log_dir + "/rotation_test";

  // Ensure log directory exists - FIXED to use C++17 filesystem
  try {
    std::filesystem::create_directories(log_dir);
  } catch (const std::exception& e) {
    std::cerr << "Failed to create log directory: " << e.what() << std::endl;
    return 1;
  }

  // Initialize log system
  if (!mm_log::Logger::Instance().Initialize(log_prefix,  // Log file prefix
          max_file_size,                                  // File size (MB)
          max_total_size,  // Total size limit (MB)
          true,            // Enable DEBUG
          true,            // Enable console
          true)) {         // Enable file
    std::cerr << "Failed to initialize log system!" << std::endl;
    return 1;
  }

  std::cout << "Starting rotation test..." << std::endl;
  std::cout << "Total logs: " << num_logs << std::endl;
  std::cout << "Log size: ~" << log_size << " characters" << std::endl;
  std::cout << "Single file size limit: " << max_file_size << " MB"
            << std::endl;
  std::cout << "Total file size limit: " << max_total_size << " MB"
            << std::endl;
  std::cout << "Log interval: " << log_interval_ms << " ms" << std::endl;
  std::cout << "Log file: " << log_prefix << ".{INFO|WARN|ERROR}" << std::endl;
  std::cout << "Press Ctrl+C to terminate test..." << std::endl;

  // Progress bar intervals
  const size_t progress_interval = num_logs / 50 > 0 ? num_logs / 50 : 1;

  // Start logging
  for (size_t i = 0; i < num_logs; ++i) {
    // Generate random sized log message
    std::string random_data = generate_random_string(log_size);

    // Randomly select log level
    int level = i % 10;  // 0-9

    if (level < 6) {  // 60% DEBUG
      MM_DEBUG("Test log #%zu [Rotation Test] Random data: %s", i,
          random_data.c_str());
    } else if (level < 8) {  // 20% INFO
      MM_INFO("Test log #%zu [Rotation Test] Random data: %s", i,
          random_data.c_str());
    } else if (level < 9) {  // 10% WARN
      MM_WARN("Test log #%zu [Rotation Test] Random data: %s", i,
          random_data.c_str());
    } else {  // 10% ERROR
      MM_ERROR("Test log #%zu [Rotation Test] Random data: %s", i,
          random_data.c_str());
    }

    // Print progress
    if (i % progress_interval == 0) {
      float progress = static_cast<float>(i) / num_logs * 100.0f;
      std::cout << "\rProgress: " << static_cast<int>(progress) << "% [";
      int pos = 50 * progress / 100;
      for (int j = 0; j < 50; ++j) {
        if (j < pos)
          std::cout << "=";
        else if (j == pos)
          std::cout << ">";
        else
          std::cout << " ";
      }
      std::cout << "] " << i << "/" << num_logs << std::flush;
    }

    // If log interval specified, wait
    if (log_interval_ms > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(log_interval_ms));
    }
  }

  std::cout << "\rProgress: 100% "
               "[==================================================] "
            << num_logs << "/" << num_logs << std::endl;
  std::cout << "Rotation test complete!" << std::endl;

  // Explicitly shutdown the logger to ensure all logs are flushed
  mm_log::Logger::Instance().Shutdown();

  return 0;
}
