#pragma once

// 包含主要的mm_logger组件
#include "mm_logger/custom_sink.hpp"
#include "mm_logger/mm_logger.hpp"

// 版本信息
#define MM_LOGGER_VERSION_MAJOR 1
#define MM_LOGGER_VERSION_MINOR 0
#define MM_LOGGER_VERSION_PATCH 0
#define MM_LOGGER_VERSION "1.0.0"

namespace mm_log {

// 获取库版本
inline const char* GetVersion() { return MM_LOGGER_VERSION; }

}  // namespace mm_log
