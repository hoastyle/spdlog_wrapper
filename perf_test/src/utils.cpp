/**
 * @file utils.cpp
 * @brief 工具函数实现
 */

#include "utils.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <random>
#include <fstream>
#include <locale>
#include <iostream>

namespace utils {

// 获取当前时间戳字符串 (YYYY-MM-DD HH:MM:SS格式)
std::string get_timestamp_str() {
  auto now        = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);

  std::stringstream ss;
  ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
  return ss.str();
}

// 创建目录（如果不存在）
bool create_directory(const std::string& path) {
  try {
    if (!std::filesystem::exists(path)) {
      return std::filesystem::create_directories(path);
    }
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Error creating directory " << path << ": " << e.what()
              << std::endl;
    return false;
  }
}

// 生成随机字符串
std::string generate_random_string(size_t length) try {
  // 限制长度，防止内存问题
  if (length > 5000) {
    std::cerr << "Warning: Requested random string length " << length
              << " is too large, limiting to 5000" << std::endl;
    length = 5000;
  }

  static const char alphanum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(0, sizeof(alphanum) - 2);

  std::string result;
  result.reserve(length + 1);  // 额外的空间用于确保容量足够

  for (size_t i = 0; i < length; ++i) {
    result += alphanum[dist(gen)];
  }

  return result;
} catch (const std::exception& e) {
  std::cerr << "Error generating random string: " << e.what() << std::endl;
  // 返回一个短的默认字符串
  return "DEFAULT_STRING_GENERATION_FAILED";
}

// 格式化数字（添加千位分隔符）
std::string format_number(double value, int precision) {
  std::stringstream ss;
  ss.imbue(std::locale(""));  // 使用本地化设置（包括千位分隔符）
  ss << std::fixed << std::setprecision(precision) << value;
  return ss.str();
}

// 检查文件是否存在
bool file_exists(const std::string& filepath) {
  return std::filesystem::exists(filepath);
}

// 获取文件大小（字节）
size_t get_file_size(const std::string& filepath) {
  if (!file_exists(filepath)) {
    return 0;
  }

  return std::filesystem::file_size(filepath);
}

// 获取进程占用内存（KB）
size_t get_process_memory_usage() {
  // 注意：这是Linux特定实现
  // 对于跨平台解决方案，需要使用特定平台API或第三方库
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

}  // namespace utils
