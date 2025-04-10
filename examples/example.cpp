#include "mm_logger.hpp"

class LandCollisionService {
public:
  void onObstacleReady() {
    // 初始化日志系统（通常在主程序开始时只调用一次）
    if (!logger_initialized_) {
      mm_log::Logger::Instance().Initialize("./logs/app_log", // 日志文件前缀
                                            5 * 1024 * 1024, // 5MB 单个文件大小
                                            20 * 1024 * 1024, // 20MB 总大小限制
                                            true,  // 启用DEBUG日志
                                            true,  // 启用控制台输出
                                            true); // 启用文件输出
      logger_initialized_ = true;
    }

    // 使用日志宏
    MM_DEBUG("[LCPS_FLOW] Input obstacle size: %d, from: %s", 5, "sensor");
    MM_INFO("Processing obstacles from sensor");
    MM_WARN("Missing some obstacle data");
    MM_ERROR("Failed to process obstacle with ID: %d", 123);
  }

private:
  static bool logger_initialized_;
};

// 静态成员初始化
bool LandCollisionService::logger_initialized_ = false;

int main() {
  LandCollisionService service;
  service.onObstacleReady();
  return 0;
}
