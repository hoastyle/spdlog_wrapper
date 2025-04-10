#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "custom_sink.hpp" // Include our custom sink
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include <cstdarg> // 用于va_list
#include <vector>  // 用于动态缓冲区

namespace mm_log {

// 定义日志级别
enum class LogLevel { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3 };

class Logger {
public:
  static Logger &Instance() {
    static Logger instance;
    return instance;
  }

  // 初始化日志系统 - 更新参数以支持总大小限制
  bool Initialize(const std::string &log_file_prefix,
                  size_t max_file_size = 10 * 1024 * 1024, // 默认10MB单文件
                  size_t max_total_size = 50 * 1024 * 1024, // 默认50MB总大小
                  bool enable_debug = true,
                  bool enable_console = true, // 默认启用控制台输出
                  bool enable_file = true) {  // 默认启用文件输出
    try {
      // 保存设置
      enable_debug_ = enable_debug;
      enable_console_ = enable_console;
      enable_file_ = enable_file;

      if (!enable_console_ && !enable_file_) {
        std::cerr << "Warning: Both console and file logging are disabled!"
                  << std::endl;
        return false;
      }

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

      // 创建并注册主日志记录器，包含所有sink
      auto logger = std::make_shared<spdlog::logger>(
          "main_logger", sinks.begin(), sinks.end());
      logger->set_level(enable_debug_ ? spdlog::level::debug
                                      : spdlog::level::info);
      spdlog::register_logger(logger);
      spdlog::set_default_logger(logger);

      initialized_ = true;
      return true;
    } catch (const spdlog::spdlog_ex &ex) {
      std::cerr << "Log initialization failed: " << ex.what() << std::endl;
      return false;
    }
  }

  // 获取源文件的基本名称（去掉路径）
  static std::string GetBaseName(const std::string &file_path) {
    size_t pos = file_path.find_last_of("/\\");
    if (pos != std::string::npos) {
      return file_path.substr(pos + 1);
    }
    return file_path;
  }

  // 通用函数，用于处理不同日志级别的格式化和日志写入
  template <typename... Args>
  void Log(LogLevel level, const char *file, const char *func, int line,
           const char *fmt, ...) {
    if (!initialized_) {
      return;
    }

    // 如果是DEBUG级别但DEBUG被禁用，则返回
    if (level == LogLevel::DEBUG && !enable_debug_) {
      return;
    }

    // 构建日志前缀（类名::函数名() 行号 级别标识）
    std::string basename = GetBaseName(file);
    std::string classname = basename.substr(0, basename.find_last_of('.'));
    std::string prefix =
        fmt::format("{}::{}() {} {}: ",
                    classname,            // 类名（提取自文件名）
                    func,                 // 函数名
                    line,                 // 行号
                    GetLevelChar(level)); // 级别标识（D/I/W/E）

    // 处理变长参数
    va_list args;
    va_start(args, fmt);

    // 估计缓冲区大小
    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(nullptr, 0, fmt, args_copy) + 1; // +1 为了结尾的 \0
    va_end(args_copy);

    std::string message;
    if (size <= 0) {
      message = "格式化错误";
    } else {
      // 分配缓冲区并格式化
      std::vector<char> buffer(size);
      vsnprintf(buffer.data(), size, fmt, args);
      message = std::string(buffer.data(),
                            buffer.data() + size - 1); // 不包括结尾的 \0
    }

    va_end(args);

    // 获取日志记录器
    auto logger = spdlog::get("main_logger");
    if (!logger) {
      return;
    }

    // 输出完整消息
    switch (level) {
    case LogLevel::DEBUG:
      logger->debug("{}{}", prefix, message);
      break;
    case LogLevel::INFO:
      logger->info("{}{}", prefix, message);
      break;
    case LogLevel::WARN:
      logger->warn("{}{}", prefix, message);
      break;
    case LogLevel::ERROR:
      logger->error("{}{}", prefix, message);
      break;
    }
  }

private:
  // 私有构造函数（单例模式）
  Logger()
      : initialized_(false), enable_debug_(true), enable_console_(true),
        enable_file_(true) {}

  ~Logger() {
    // 刷新所有日志
    if (initialized_) {
      spdlog::shutdown();
    }
  }

  // 获取日志级别对应的字符
  static char GetLevelChar(LogLevel level) {
    switch (level) {
    case LogLevel::DEBUG:
      return 'D';
    case LogLevel::INFO:
      return 'I';
    case LogLevel::WARN:
      return 'W';
    case LogLevel::ERROR:
      return 'E';
    default:
      return '?';
    }
  }

  // 格式化函数 (printf风格)
  static std::string FormatString(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    // 估计缓冲区大小
    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(nullptr, 0, fmt, args_copy) + 1; // +1 为了结尾的 \0
    va_end(args_copy);

    if (size <= 0) {
      va_end(args);
      return "格式化错误";
    }

    // 分配缓冲区并格式化
    std::vector<char> buffer(size);
    vsnprintf(buffer.data(), size, fmt, args);
    va_end(args);

    return std::string(buffer.data(),
                       buffer.data() + size - 1); // 不包括结尾的 \0
  }

  // 成员变量
  bool initialized_;
  bool enable_debug_;
  bool enable_console_;
  bool enable_file_;
};

} // namespace mm_log

#define MM_DEBUG(fmt, ...)                                                     \
  mm_log::Logger::Instance().Log(mm_log::LogLevel::DEBUG, __FILE__,            \
                                 __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

#define MM_INFO(fmt, ...)                                                      \
  mm_log::Logger::Instance().Log(mm_log::LogLevel::INFO, __FILE__,             \
                                 __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

#define MM_WARN(fmt, ...)                                                      \
  mm_log::Logger::Instance().Log(mm_log::LogLevel::WARN, __FILE__,             \
                                 __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

#define MM_ERROR(fmt, ...)                                                     \
  mm_log::Logger::Instance().Log(mm_log::LogLevel::ERROR, __FILE__,            \
                                 __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
