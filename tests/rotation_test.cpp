#include "mm_logger.hpp"
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <thread>

// 生成随机字符串函数
std::string generate_random_string(size_t length) {
  static const char alphanum[] = "0123456789"
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

int main(int argc, char *argv[]) {
  // 解析命令行参数
  size_t num_logs = 10000;  // 默认日志条数
  size_t log_size = 100;    // 默认每条日志大小（字符数）
  size_t max_file_size = 1; // 默认轮转文件大小（MB）
  size_t max_files = 5;     // 默认最大文件数
  int log_interval_ms = 0;  // 默认无延迟

  // 检查命令行参数
  if (argc > 1)
    num_logs = std::stoul(argv[1]);
  if (argc > 2)
    log_size = std::stoul(argv[2]);
  if (argc > 3)
    max_file_size = std::stoul(argv[3]);
  if (argc > 4)
    max_files = std::stoul(argv[4]);
  if (argc > 5)
    log_interval_ms = std::stoi(argv[5]);

  // 初始化日志系统
  std::string log_dir = "./logs";
  std::string log_prefix = log_dir + "/rotation_test";

// 确保日志目录存在
#ifdef _WIN32
  system(("mkdir " + log_dir + " 2>nul").c_str());
#else
  system(("mkdir -p " + log_dir).c_str());
#endif

  // 初始化日志系统
  if (!mm_log::Logger::Instance().Initialize(log_prefix, // 日志文件前缀
                                             max_file_size * 1024 *
                                                 1024, // 最大文件大小（字节）
                                             max_files, // 最大文件数
                                             true,      // 启用DEBUG
                                             true,      // 启用控制台
                                             true)) {   // 启用文件
    std::cerr << "日志系统初始化失败！" << std::endl;
    return 1;
  }

  std::cout << "开始轮转测试..." << std::endl;
  std::cout << "总日志条数: " << num_logs << std::endl;
  std::cout << "每条日志大小: ~" << log_size << " 字符" << std::endl;
  std::cout << "轮转文件大小: " << max_file_size << " MB" << std::endl;
  std::cout << "最大文件数: " << max_files << std::endl;
  std::cout << "日志间隔: " << log_interval_ms << " ms" << std::endl;
  std::cout << "日志文件: " << log_prefix << ".{INFO|WARN|ERROR}" << std::endl;
  std::cout << "按Ctrl+C终止测试..." << std::endl;

  // 打印进度条的间隔数量
  const size_t progress_interval = num_logs / 50 > 0 ? num_logs / 50 : 1;

  // 开始记录日志
  for (size_t i = 0; i < num_logs; ++i) {
    // 生成随机大小的日志消息
    std::string random_data = generate_random_string(log_size);

    // 随机选择日志级别
    int level = i % 10; // 0-9

    if (level < 6) { // 60% DEBUG
      MM_DEBUG("测试日志 #%zu [轮转测试] 随机数据: %s", i, random_data.c_str());
    } else if (level < 8) { // 20% INFO
      MM_INFO("测试日志 #%zu [轮转测试] 随机数据: %s", i, random_data.c_str());
    } else if (level < 9) { // 10% WARN
      MM_WARN("测试日志 #%zu [轮转测试] 随机数据: %s", i, random_data.c_str());
    } else { // 10% ERROR
      MM_ERROR("测试日志 #%zu [轮转测试] 随机数据: %s", i, random_data.c_str());
    }

    // 打印进度
    if (i % progress_interval == 0) {
      float progress = static_cast<float>(i) / num_logs * 100.0f;
      std::cout << "\r进度: " << static_cast<int>(progress) << "% [";
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

    // 如果指定了日志间隔，则等待
    if (log_interval_ms > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(log_interval_ms));
    }
  }

  std::cout
      << "\r进度: 100% [==================================================] "
      << num_logs << "/" << num_logs << std::endl;
  std::cout << "轮转测试完成!" << std::endl;

  return 0;
}
