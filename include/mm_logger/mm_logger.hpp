#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>

#include "custom_sink.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/async.h"  // 引入异步日志支持
#include "spdlog/spdlog.h"
#include <cstdarg>  // 用于va_list
#include <vector>   // 用于动态缓冲区

namespace mm_log {

// 定义日志级别
enum class LogLevel { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3 };

class Logger {
 public:
  static Logger& Instance() {
    static Logger instance;
    return instance;
  }

  // 初始化日志系统 - 更新参数以支持总大小限制和异步日志
  bool Initialize(const std::string& log_file_prefix, size_t max_file_size = 10,
      size_t max_total_size = 50, bool enable_debug = false,
      bool enable_console = false, bool enable_file = true,
      size_t queue_size = 8192, size_t thread_count = 1) {
    // 使用std::call_once确保Initialize只被调用一次
    std::call_once(init_flag_, [&]() {
      try {
        // 保存设置
        enable_debug_   = enable_debug;
        enable_console_ = enable_console;
        enable_file_    = enable_file;

        if (!enable_console_ && !enable_file_) {
          std::cerr << "Warning: Both console and file logging are disabled!"
                    << std::endl;
          initialized_.store(false);
          return;
        }

        // 初始化全局异步日志
        spdlog::init_thread_pool(queue_size, thread_count);

        // 创建多重日志记录器，将合并不同的输出目标
        std::vector<spdlog::sink_ptr> sinks;

        // 添加控制台输出
        if (enable_console_) {
          auto console_sink =
              std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
          console_sink->set_pattern("%P:I%Y%m%d %H:%M:%S.%f %t | %v");
          sinks.push_back(console_sink);
        }

        // 添加文件输出
        if (enable_file_) {
          // 创建INFO文件sink（包含所有级别）
          auto info_sink = std::make_shared<custom_rotating_file_sink_mt>(
              log_file_prefix, "INFO", max_file_size, max_total_size);
          info_sink->set_pattern("%P:I%Y%m%d %H:%M:%S.%f %t :0] %v");
          info_sink->set_level(spdlog::level::debug);
          sinks.push_back(info_sink);

          // 创建WARN文件sink（只包含WARN和ERROR级别）
          auto warn_sink = std::make_shared<custom_rotating_file_sink_mt>(
              log_file_prefix, "WARN", max_file_size, max_total_size);
          warn_sink->set_pattern("%P:I%Y%m%d %H:%M:%S.%f %t :0] %v");
          warn_sink->set_level(spdlog::level::warn);
          sinks.push_back(warn_sink);

          // 创建ERROR文件sink（只包含ERROR级别）
          auto error_sink = std::make_shared<custom_rotating_file_sink_mt>(
              log_file_prefix, "ERROR", max_file_size, max_total_size);
          error_sink->set_pattern("%P:I%Y%m%d %H:%M:%S.%f %t :0] %v");
          error_sink->set_level(spdlog::level::err);
          sinks.push_back(error_sink);
        }

        // 创建并注册异步日志记录器
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
      double max_file_size_gb  = 0.01,  // 默认10MB
      double max_total_size_gb = 0.05,  // 默认50MB
      bool enable_debug = true, bool enable_console = true,
      bool enable_file = true, size_t queue_size = 8192,
      size_t thread_count = 1) {
    size_t max_file_size_mb  = static_cast<size_t>(max_file_size_gb * 1024.0);
    size_t max_total_size_mb = static_cast<size_t>(max_total_size_gb * 1024.0);

    return Initialize(log_file_prefix, max_file_size_mb, max_total_size_mb,
        enable_debug, enable_console, enable_file, queue_size, thread_count);
  }

  // 获取源文件的基本名称（去掉路径）
  static std::string GetBaseName(const std::string& file_path) {
    size_t pos = file_path.find_last_of("/\\");
    if (pos != std::string::npos) {
      return file_path.substr(pos + 1);
    }
    return file_path;
  }

  // 通用函数，用于处理不同日志级别的格式化和日志写入
  template <typename... Args>
  void Log(LogLevel level, const char* file, const char* func, int line,
      const char* fmt, ...) {
    if (!initialized_.load()) {
      return;
    }

    // 如果是DEBUG级别但DEBUG被禁用，则返回
    if (level == LogLevel::DEBUG && !enable_debug_) {
      return;
    }

    // 构建日志前缀（类名::函数名() 行号 级别标识）
    std::string basename  = GetBaseName(file);
    std::string classname = basename.substr(0, basename.find_last_of('.'));
    std::string prefix    = fmt::format("{}::{}() {} {}: ",
        classname,             // 类名（提取自文件名）
        func,                  // 函数名
        line,                  // 行号
        GetLevelChar(level));  // 级别标识（D/I/W/E）

    // 处理变长参数
    va_list args;
    va_start(args, fmt);

    // 估计缓冲区大小
    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(nullptr, 0, fmt, args_copy) + 1;  // +1 为了结尾的 \0
    va_end(args_copy);

    std::string message;
    if (size <= 0) {
      message = "格式化错误";
    } else {
      // 分配缓冲区并格式化
      std::vector<char> buffer(size);
      vsnprintf(buffer.data(), size, fmt, args);
      message = std::string(buffer.data(),
          buffer.data() + size - 1);  // 不包括结尾的 \0
    }

    va_end(args);

    // 获取日志记录器
    auto logger = spdlog::get("main_logger");
    if (!logger) {
      return;
    }

    // 输出完整消息
    switch (level) {
      case LogLevel::DEBUG: logger->debug("{}{}", prefix, message); break;
      case LogLevel::INFO: logger->info("{}{}", prefix, message); break;
      case LogLevel::WARN: logger->warn("{}{}", prefix, message); break;
      case LogLevel::ERROR: logger->error("{}{}", prefix, message); break;
    }
  }

  // 在程序结束前主动刷新和关闭日志
  void Shutdown() {
    if (initialized_.load()) {
      spdlog::shutdown();
      initialized_.store(false);
    }
  }

 private:
  // 私有构造函数（单例模式）
  Logger()
      : initialized_(false),
        enable_debug_(true),
        enable_console_(true),
        enable_file_(true) {}

  ~Logger() {
    // 刷新所有日志
    Shutdown();
  }

  // 获取日志级别对应的字符
  static char GetLevelChar(LogLevel level) {
    switch (level) {
      case LogLevel::DEBUG: return 'D';
      case LogLevel::INFO: return 'I';
      case LogLevel::WARN: return 'W';
      case LogLevel::ERROR: return 'E';
      default: return '?';
    }
  }

  // 成员变量
  std::atomic<bool> initialized_;  // 使用atomic保证线程安全
  std::once_flag init_flag_;       // 确保初始化只执行一次
  bool enable_debug_;
  bool enable_console_;
  bool enable_file_;
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
