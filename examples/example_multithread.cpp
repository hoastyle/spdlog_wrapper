#include "mm_logger/mm_logger.hpp"
#include <thread>
#include <vector>
#include <atomic>
#include <iomanip>
#include <chrono>
#include <random>
#include <condition_variable>

// 线程同步工具，确保所有线程同时开始
class ThreadBarrier {
 public:
  explicit ThreadBarrier(size_t count)
      : threshold_(count), count_(count), generation_(0) {}

  void wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    auto generation = generation_;
    if (--count_ == 0) {
      // 最后一个线程到达屏障，重置count并唤醒所有等待线程
      count_ = threshold_;
      ++generation_;
      cv_.notify_all();
    } else {
      // 等待最后一个线程
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

// 日志性能统计类
class LoggerPerfStats {
 public:
  static LoggerPerfStats& Instance() {
    static LoggerPerfStats instance;
    return instance;
  }

  void incrementLogCount() { total_logs_++; }

  void startTest() { start_time_ = std::chrono::high_resolution_clock::now(); }

  void endTest() {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time_)
                        .count();
    std::cout << "---------------------------------------------\n";
    std::cout << "Performance Statistics:\n";
    std::cout << "Total logs: " << total_logs_.load() << "\n";
    std::cout << "Test duration: " << duration << " ms\n";

    double logs_per_second =
        static_cast<double>(total_logs_.load()) * 1000.0 / duration;
    std::cout << "Logs per second: " << std::fixed << std::setprecision(2)
              << logs_per_second << "\n";
    std::cout << "---------------------------------------------\n";
  }

 private:
  LoggerPerfStats() : total_logs_(0) {}
  std::atomic<uint64_t> total_logs_;
  std::chrono::high_resolution_clock::time_point start_time_;
};

// 工作线程函数
void worker_thread(int thread_id, int iterations, ThreadBarrier& barrier,
    bool use_random_delay) {
  // 准备随机延迟生成器
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> delay_dist(1, 5);

  // 等待所有线程准备就绪
  barrier.wait();

  for (int i = 0; i < iterations; ++i) {
    // 生成一些模拟数据
    int data_size = 10 + (thread_id % 15);
    int sensor_id = 100 + (thread_id % 50);

    // 记录不同级别的日志
    MM_DEBUG(
        "[Thread-%03d] Iteration %d/%d: Processing data from sensor %d with "
        "size %d",
        thread_id, i + 1, iterations, sensor_id, data_size);
    LoggerPerfStats::Instance().incrementLogCount();

    if (i % 10 == 0) {
      MM_INFO("[Thread-%03d] Processed %d/%d iterations with %d data items",
          thread_id, i + 1, iterations, data_size);
      LoggerPerfStats::Instance().incrementLogCount();
    }

    if (i % 50 == 0) {
      MM_WARN(
          "[Thread-%03d] Warning: Sensor %d reading is unstable at iteration "
          "%d",
          thread_id, sensor_id, i + 1);
      LoggerPerfStats::Instance().incrementLogCount();
    }

    if (i % 200 == 0) {
      MM_ERROR(
          "[Thread-%03d] Error: Failed to process data from sensor %d at "
          "iteration %d",
          thread_id, sensor_id, i + 1);
      LoggerPerfStats::Instance().incrementLogCount();
    }

    // 可选的随机延迟，模拟真实工作负载
    if (use_random_delay) {
      std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(gen)));
    }
  }
}

int main(int argc, char* argv[]) {
  // 默认参数
  int num_threads       = 100;   // 默认线程数
  int iterations        = 100;   // 每线程迭代次数
  bool use_random_delay = true;  // 是否使用随机延迟
  int queue_size        = 8192;  // 异步队列大小
  int worker_threads    = 2;     // 日志工作线程数
  bool enable_console   = true;  // 是否启用控制台输出

  // 解析命令行参数
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--threads" && i + 1 < argc) {
      num_threads = std::stoi(argv[++i]);
    } else if (arg == "--iterations" && i + 1 < argc) {
      iterations = std::stoi(argv[++i]);
    } else if (arg == "--no-delay") {
      use_random_delay = false;
    } else if (arg == "--queue-size" && i + 1 < argc) {
      queue_size = std::stoi(argv[++i]);
    } else if (arg == "--worker-threads" && i + 1 < argc) {
      worker_threads = std::stoi(argv[++i]);
    } else if (arg == "--no-console") {
      enable_console = false;
    } else if (arg == "--help") {
      std::cout
          << "Usage: " << argv[0] << " [options]\n"
          << "Options:\n"
          << "  --threads N          Number of threads (default: 100)\n"
          << "  --iterations N       Iterations per thread (default: 100)\n"
          << "  --no-delay           Disable random delays between logs\n"
          << "  --queue-size N       Async log queue size (default: 8192)\n"
          << "  --worker-threads N   Logger worker threads (default: 2)\n"
          << "  --no-console         Disable console output\n"
          << "  --help               Show this help message\n";
      return 0;
    }
  }

  std::cout << "Starting multi-threaded logger test\n";
  std::cout << "Threads: " << num_threads << "\n";
  std::cout << "Iterations per thread: " << iterations << "\n";
  std::cout << "Random delay: " << (use_random_delay ? "enabled" : "disabled")
            << "\n";
  std::cout << "Queue size: " << queue_size << "\n";
  std::cout << "Worker threads: " << worker_threads << "\n";
  std::cout << "Console output: " << (enable_console ? "enabled" : "disabled")
            << "\n";

  // 初始化日志系统
  if (!mm_log::Logger::Instance().Initialize("./logs/multithread_test",
          5,                  // 5MB 文件大小
          50,                 // 50MB 总大小限制
          true,               // 启用DEBUG
          enable_console,     // 控制台输出由参数决定
          true,               // 始终启用文件输出
          queue_size,         // 异步队列大小
          worker_threads)) {  // 后台工作线程数
    std::cerr << "Failed to initialize logger!" << std::endl;
    return 1;
  }

  // 创建线程屏障，确保所有线程同时开始
  ThreadBarrier barrier(num_threads);

  // 创建并启动工作线程
  std::vector<std::thread> threads;
  threads.reserve(num_threads);

  std::cout << "Preparing threads...\n";

  // 创建所有工作线程
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(
        worker_thread, i, iterations, std::ref(barrier), use_random_delay);
  }

  std::cout << "All threads created. Starting test...\n";

  // 开始性能测试计时
  LoggerPerfStats::Instance().startTest();

  // 等待所有线程完成
  for (auto& t : threads) {
    if (t.joinable()) {
      t.join();
    }
  }

  // 结束测试并输出性能统计
  LoggerPerfStats::Instance().endTest();

  // 关闭日志系统
  mm_log::Logger::Instance().Shutdown();

  std::cout << "Test completed successfully.\n";

  return 0;
}
