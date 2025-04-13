/**
 * @file utils.h
 * @brief 工具函数定义
 */

#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace utils {

// 获取当前时间戳字符串
std::string get_timestamp_str();

// 创建目录（如果不存在）
bool create_directory(const std::string& path);

// 生成随机字符串
std::string generate_random_string(size_t length);

// 统计性能差异百分比
double calculate_percentage_diff(double value1, double value2);

// 格式化数字（添加千位分隔符）
std::string format_number(double value, int precision = 2);

// 检查文件是否存在
bool file_exists(const std::string& filepath);

// 获取文件大小（字节）
size_t get_file_size(const std::string& filepath);

// 获取进程占用内存（KB）
size_t get_process_memory_usage();

}  // namespace utils
