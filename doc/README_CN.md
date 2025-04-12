# MM-Logger

一个高性能、多线程安全的C++日志库，基于spdlog。

## 特性

- 多线程安全设计
- 异步日志支持
- 自动文件轮转
- 基于总大小的日志管理
- 支持多种日志级别 (DEBUG, INFO, WARN, ERROR)
- 格式化输出 (printf风格)
- 支持控制台和文件输出

## 要求

- C++17 或更高版本
- CMake 3.10 或更高版本
- spdlog 库（如果系统中没有安装，将自动下载）

## 安装

### 方法1：从源码构建

```bash
git clone https://github.com/yourusername/mm_logger.git
cd mm_logger
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
sudo make install
```

### 方法2：使用预编译包

从[Releases](https://github.com/yourusername/mm_logger/releases)页面下载最新的预编译包，然后解压并安装：

```bash
tar -xzf mm_logger-1.0.0-linux.tar.gz -C /tmp/
sudo cp -r /tmp/include/* /usr/local/include/
sudo cp -r /tmp/lib/* /usr/local/lib/
sudo ldconfig
```

## 在项目中使用

### 使用CMake

在你的CMakeLists.txt中添加：

```cmake
find_package(mm_logger REQUIRED)
target_link_libraries(your_target PRIVATE mm_logger::mm_logger)
```

### 代码示例

```cpp
#include <mm_logger/mm_logger_all.hpp>

int main() {
    // 初始化日志系统
    mm_log::Logger::Instance().Initialize(
        "./logs/my_app",      // 日志文件前缀
        5,      // 5MB 单个文件大小
        20,     // 20MB 总大小限制
        true,   // 启用DEBUG日志
        true,   // 启用控制台输出
        true    // 启用文件输出
    );
    
    // 使用日志宏
    MM_DEBUG("这是一条调试日志 %s", "详细信息");
    MM_INFO("这是一条信息日志");
    MM_WARN("这是一条警告日志");
    MM_ERROR("这是一条错误日志 #%d", 123);
    
    return 0;
}
```

## 高级配置

### 异步日志

```cpp
// 使用异步日志，配置队列大小和工作线程数
mm_log::Logger::Instance().Initialize(
    "./logs/my_app",
    5,
    20,
    true,
    true,
    true,
    8192,   // 异步队列大小
    2       // 工作线程数
);
```

### 禁用控制台输出（提高性能）

```cpp
mm_log::Logger::Instance().Initialize(
    "./logs/my_app",
    5,
    20,
    true,
    false,  // 禁用控制台输出
    true
);
```

## 许可证

MIT License
