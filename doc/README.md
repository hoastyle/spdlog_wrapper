# MM-Logger

A high-performance, thread-safe C++ logging library based on spdlog.

## Features

- Thread-safe design for high-concurrency environments
- Asynchronous logging support
- Automatic file rotation
- Log management based on total size
- Multiple log levels (DEBUG, INFO, WARN, ERROR)
- Formatted output (printf style)
- Support for both console and file output
- Symbolic links to the latest log files

## Requirements

- C++17 or higher
- CMake 3.10 or higher
- spdlog library (will be automatically downloaded if not installed)

## Installation

### Method 1: Build from source

```bash
git clone https://github.com/yourusername/mm_logger.git
cd mm_logger
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
sudo make install
```

### Method 2: Use pre-compiled package

Download the latest pre-compiled package from the [Releases](https://github.com/yourusername/mm_logger/releases) page, then extract and install:

```bash
tar -xzf mm_logger-1.0.0-linux.tar.gz -C /tmp/
sudo cp -r /tmp/include/* /usr/local/include/
sudo cp -r /tmp/lib/* /usr/local/lib/
sudo ldconfig
```

## Usage in Your Projects

### Using with CMake

Add to your CMakeLists.txt:

```cmake
find_package(mm_logger REQUIRED)
target_link_libraries(your_target PRIVATE mm_logger::mm_logger)
```

### Code Example

```cpp
#include <mm_logger/mm_logger_all.hpp>

int main() {
    // Initialize the logging system
    mm_log::Logger::Instance().Initialize(
        "./logs/my_app",      // Log file prefix
        5 * 1024 * 1024,      // 5MB single file size
        20 * 1024 * 1024,     // 20MB total size limit
        true,                 // Enable DEBUG logs
        true,                 // Enable console output
        true                  // Enable file output
    );
    
    // Use logging macros
    MM_DEBUG("This is a debug log: %s", "details");
    MM_INFO("This is an info log");
    MM_WARN("This is a warning log");
    MM_ERROR("This is an error log: #%d", 123);
    
    return 0;
}
```

## Advanced Configuration

### Asynchronous Logging

For high-performance environments with many concurrent threads:

```cpp
// Use async logging with queue size and worker threads configuration
mm_log::Logger::Instance().Initialize(
    "./logs/my_app",
    5 * 1024 * 1024,
    20 * 1024 * 1024,
    true,
    true,
    true,
    8192,   // Async queue size
    2       // Number of worker threads
);
```

### Disable Console Output for Better Performance

```cpp
mm_log::Logger::Instance().Initialize(
    "./logs/my_app",
    5 * 1024 * 1024,
    20 * 1024 * 1024,
    true,
    false,  // Disable console output
    true
);
```

## Thread Safety and Performance

MM-Logger is designed to handle hundreds of concurrent threads writing logs simultaneously. Key features include:

- Thread-safe singleton pattern with atomic initialization
- Lock-free operations where possible
- Asynchronous logging with configurable queue size and worker threads
- Optimized file rotation for minimum lock contention
- Efficient memory usage with minimal allocations per log entry

## Log File Management

Logs are automatically managed with:

- Size-based rotation with configurable file size limits
- Total size limits with automatic cleanup of older files
- Timestamp-based file naming for easy identification
- Symbolic links to the most recent logs for each level

## License

MIT License
