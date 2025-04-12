#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>

#include "custom_sink.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/async.h"  // Include async logging support
#include "spdlog/spdlog.h"
#include <cstdarg>  // For va_list
#include <vector>   // For dynamic buffer

namespace mm_log {

// Define log levels
enum class LogLevel { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3 };

class Logger {
 public:
  static Logger& Instance() {
    static Logger instance;
    return instance;
  }

  // Initialize logging system - with parameters for total size limit and async
  // logging
  bool Initialize(const std::string& log_file_prefix, size_t max_file_size = 10,
      size_t max_total_size = 50, bool enable_debug = false,
      bool enable_console = false, bool enable_file = true,
      size_t queue_size = 8192, size_t thread_count = 1) {
    // Use std::call_once to ensure Initialize is called only once
    std::call_once(init_flag_, [&]() {
      try {
        // Save settings
        enable_debug_   = enable_debug;
        enable_console_ = enable_console;
        enable_file_    = enable_file;
        current_level_  = enable_debug ? LogLevel::DEBUG : LogLevel::INFO;

        if (!enable_console_ && !enable_file_) {
          std::cerr << "Warning: Both console and file logging are disabled!"
                    << std::endl;
          initialized_.store(false);
          return;
        }

        // Initialize global async logging
        spdlog::init_thread_pool(queue_size, thread_count);

        // Create multiple logger that will merge different output targets
        std::vector<spdlog::sink_ptr> sinks;

        // Add console output
        if (enable_console_) {
          auto console_sink =
              std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
          console_sink->set_pattern("%P:I%Y%m%d %H:%M:%S.%f %t | %v");
          sinks.push_back(console_sink);
        }

        // Add file output
        if (enable_file_) {
          // Create INFO file sink (includes all levels)
          auto info_sink = std::make_shared<custom_rotating_file_sink_mt>(
              log_file_prefix, "INFO", max_file_size, max_total_size);
          info_sink->set_pattern("%P:I%Y%m%d %H:%M:%S.%f %t :0] %v");
          info_sink->set_level(spdlog::level::debug);
          sinks.push_back(info_sink);

          // Create WARN file sink (only includes WARN and ERROR levels)
          auto warn_sink = std::make_shared<custom_rotating_file_sink_mt>(
              log_file_prefix, "WARN", max_file_size, max_total_size);
          warn_sink->set_pattern("%P:I%Y%m%d %H:%M:%S.%f %t :0] %v");
          warn_sink->set_level(spdlog::level::warn);
          sinks.push_back(warn_sink);

          // Create ERROR file sink (only includes ERROR level)
          auto error_sink = std::make_shared<custom_rotating_file_sink_mt>(
              log_file_prefix, "ERROR", max_file_size, max_total_size);
          error_sink->set_pattern("%P:I%Y%m%d %H:%M:%S.%f %t :0] %v");
          error_sink->set_level(spdlog::level::err);
          sinks.push_back(error_sink);
        }

        // Create and register async logger
        auto logger = std::make_shared<spdlog::async_logger>("main_logger",
            sinks.begin(), sinks.end(), spdlog::thread_pool(),
            spdlog::async_overflow_policy::block);

        logger->set_level(
            enable_debug_ ? spdlog::level::debug : spdlog::level::info);
        spdlog::register_logger(logger);
        spdlog::set_default_logger(logger);

        initialized_.store(true);
      } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        initialized_.store(false);
      }
    });

    return initialized_.load();
  }

  bool InitializeWithGB(const std::string& log_file_prefix,
      double max_file_size_gb  = 0.01,  // Default 10MB
      double max_total_size_gb = 0.05,  // Default 50MB
      bool enable_debug = true, bool enable_console = true,
      bool enable_file = true, size_t queue_size = 8192,
      size_t thread_count = 1) {
    size_t max_file_size_mb  = static_cast<size_t>(max_file_size_gb * 1024.0);
    size_t max_total_size_mb = static_cast<size_t>(max_total_size_gb * 1024.0);

    return Initialize(log_file_prefix, max_file_size_mb, max_total_size_mb,
        enable_debug, enable_console, enable_file, queue_size, thread_count);
  }

  // New: Method to dynamically set log level
  bool SetLogLevel(LogLevel level) {
    if (!initialized_.load()) {
      return false;
    }

    // Update current log level
    current_level_.store(level);

    // Get spdlog logger and update its level
    auto logger = spdlog::get("main_logger");
    if (!logger) {
      return false;
    }

    // Convert mm_log level to spdlog level
    spdlog::level::level_enum spdlog_level;
    switch (level) {
      case LogLevel::DEBUG: spdlog_level = spdlog::level::debug; break;
      case LogLevel::INFO: spdlog_level = spdlog::level::info; break;
      case LogLevel::WARN: spdlog_level = spdlog::level::warn; break;
      case LogLevel::ERROR: spdlog_level = spdlog::level::err; break;
      default: spdlog_level = spdlog::level::info;
    }

    // Set new level
    logger->set_level(spdlog_level);
    return true;
  }

  // New: Get current log level
  LogLevel GetLogLevel() const { return current_level_.load(); }

  // Get base name of source file (remove path)
  static std::string GetBaseName(const std::string& file_path) {
    size_t pos = file_path.find_last_of("/\\");
    if (pos != std::string::npos) {
      return file_path.substr(pos + 1);
    }
    return file_path;
  }

  // General function for handling formatting and logging at different levels
  template <typename... Args>
  void Log(LogLevel level, const char* file, const char* func, int line,
      const char* fmt, ...) {
    if (!initialized_.load()) {
      return;
    }

    // Check if log level should be recorded
    if (level < current_level_.load()) {
      return;
    }

    // Build log prefix (classname::functionname() line_number level_indicator)
    std::string basename  = GetBaseName(file);
    std::string classname = basename.substr(0, basename.find_last_of('.'));
    std::string prefix    = fmt::format("{}::{}() {} {}: ",
           classname,             // Class name (extracted from file name)
           func,                  // Function name
           line,                  // Line number
           GetLevelChar(level));  // Level indicator (D/I/W/E)

    // Handle variable arguments
    va_list args;
    va_start(args, fmt);

    // Estimate buffer size
    va_list args_copy;
    va_copy(args_copy, args);
    int size =
        vsnprintf(nullptr, 0, fmt, args_copy) + 1;  // +1 for terminating \0
    va_end(args_copy);

    std::string message;
    if (size <= 0) {
      message = "Format error";
    } else {
      // Allocate buffer and format
      std::vector<char> buffer(size);
      vsnprintf(buffer.data(), size, fmt, args);
      message = std::string(buffer.data(),
          buffer.data() + size - 1);  // Exclude terminating \0
    }

    va_end(args);

    // Get logger
    auto logger = spdlog::get("main_logger");
    if (!logger) {
      return;
    }

    // Output complete message
    switch (level) {
      case LogLevel::DEBUG: logger->debug("{}{}", prefix, message); break;
      case LogLevel::INFO: logger->info("{}{}", prefix, message); break;
      case LogLevel::WARN: logger->warn("{}{}", prefix, message); break;
      case LogLevel::ERROR: logger->error("{}{}", prefix, message); break;
    }
  }

  // Actively flush and close logs before program ends
  void Shutdown() {
    if (initialized_.load()) {
      spdlog::shutdown();
      initialized_.store(false);
    }
  }

 private:
  // Private constructor (singleton pattern)
  Logger()
      : initialized_(false),
        enable_debug_(true),
        enable_console_(true),
        enable_file_(true),
        current_level_(LogLevel::INFO) {}

  ~Logger() {
    // Flush all logs
    Shutdown();
  }

  // Get character corresponding to log level
  static char GetLevelChar(LogLevel level) {
    switch (level) {
      case LogLevel::DEBUG: return 'D';
      case LogLevel::INFO: return 'I';
      case LogLevel::WARN: return 'W';
      case LogLevel::ERROR: return 'E';
      default: return '?';
    }
  }

  // Member variables
  std::atomic<bool> initialized_;  // Use atomic to ensure thread safety
  std::once_flag init_flag_;  // Ensure initialization is executed only once
  bool enable_debug_;
  bool enable_console_;
  bool enable_file_;
  std::atomic<LogLevel>
      current_level_;  // Current log level, atomic for thread safety
};

}  // namespace mm_log

#define MM_DEBUG(fmt, ...)                                          \
  mm_log::Logger::Instance().Log(mm_log::LogLevel::DEBUG, __FILE__, \
      __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

#define MM_INFO(fmt, ...)                                          \
  mm_log::Logger::Instance().Log(mm_log::LogLevel::INFO, __FILE__, \
      __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

#define MM_WARN(fmt, ...)                                          \
  mm_log::Logger::Instance().Log(mm_log::LogLevel::WARN, __FILE__, \
      __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

#define MM_ERROR(fmt, ...)                                          \
  mm_log::Logger::Instance().Log(mm_log::LogLevel::ERROR, __FILE__, \
      __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
