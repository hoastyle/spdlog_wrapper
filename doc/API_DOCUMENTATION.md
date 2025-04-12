# MM-Logger API Documentation

## Overview

MM-Logger is a high-performance thread-safe logging library for C++ applications, particularly designed for multi-threaded environments with high log volumes. This document provides detailed API reference and usage examples.

## Core Components

### Logger Class

The main interface for interacting with the logging system.

```cpp
namespace mm_log {

class Logger {
public:
    // Singleton accessor
    static Logger& Instance();

    // Initialize the logging system
    bool Initialize(
        const std::string& log_file_prefix,  // Log file prefix
        size_t max_file_size = 10 * 1024 * 1024,  // 10MB default file size
        size_t max_total_size = 50 * 1024 * 1024,  // 50MB default total size
        bool enable_debug = true,  // Enable DEBUG level logs
        bool enable_console = true,  // Enable console output
        bool enable_file = true,  // Enable file output
        size_t queue_size = 8192,  // Async queue size
        size_t thread_count = 1  // Async worker threads
    );

    // Log a message with the specified level
    template <typename... Args>
    void Log(LogLevel level, const char* file, const char* func, int line,
             const char* fmt, ...);

    // Shutdown the logging system and flush all pending logs
    void Shutdown();
};

} // namespace mm_log
```

### Log Levels

```cpp
enum class LogLevel {
    DEBUG = 0,  // Detailed debugging information
    INFO = 1,   // General information messages
    WARN = 2,   // Warning messages that don't affect operation
    ERROR = 3    // Error messages that affect operation
};
```

### Logging Macros

For convenience, the library provides macros for each log level:

```cpp
// Debug level logging
MM_DEBUG(fmt, ...);

// Info level logging
MM_INFO(fmt, ...);

// Warning level logging
MM_WARN(fmt, ...);

// Error level logging  
MM_ERROR(fmt, ...);
```

## Initialization Parameters

When initializing the logger, you can configure various parameters:

| Parameter | Description | Default |
|-----------|-------------|---------|
| `log_file_prefix` | Path and prefix for log files | *Required* |
| `max_file_size` | Maximum size in bytes for a single log file | 10MB |
| `max_total_size` | Maximum total size in bytes for all log files | 50MB | 
| `enable_debug` | Whether to output DEBUG level logs | true |
| `enable_console` | Whether to output logs to console | true |
| `enable_file` | Whether to output logs to files | true |
| `queue_size` | Size of the async logging queue | 8192 |
| `thread_count` | Number of worker threads for async logging | 1 |

## File Naming Conventions

Log files follow this naming pattern:
```
[log_file_prefix].[LEVEL]             # Symbolic link to latest log file
[directory]/[LEVEL].[timestamp].[basename]  # Actual log files
```

Example:
```
./logs/my_app.INFO             # Link to latest INFO log
./logs/INFO.20250412_143045.my_app  # Actual log file
```

## Usage Examples

### Basic Initialization

```cpp
#include <mm_logger/mm_logger_all.hpp>

// Initialize at program start
bool init_success = mm_log::Logger::Instance().Initialize(
    "./logs/my_app",      // Log files will be in ./logs/ directory
    5 * 1024 * 1024,      // 5MB per file
    20 * 1024 * 1024,     // 20MB total
    true,                 // Enable DEBUG logs
    true,                 // Enable console output
    true                  // Enable file output
);

if (!init_success) {
    std::cerr << "Failed to initialize logger!" << std::endl;
    return 1;
}
```

### Using Log Macros in Classes

```cpp
class MyService {
public:
    void processData(const std::string& data) {
        MM_INFO("Processing data with size: %zu", data.size());
        
        try {
            // Process data...
            MM_DEBUG("Data processing details: %s", "some details");
        } 
        catch (const std::exception& e) {
            MM_ERROR("Failed to process data: %s", e.what());
        }
    }
};
```

### High-Concurrency Configuration

For applications with hundreds of threads:

```cpp
mm_log::Logger::Instance().Initialize(
    "./logs/high_concurrency_app",
    5 * 1024 * 1024,      // 5MB per file
    100 * 1024 * 1024,    // 100MB total (larger for high volume)
    true,                 // Enable DEBUG
    false,                // Disable console for performance
    true,                 // Enable file output
    32768,                // Larger queue (32K entries)
    4                     // 4 worker threads
);
```

### Shutting Down Gracefully

```cpp
// At program end
mm_log::Logger::Instance().Shutdown();
```

## Performance Considerations

- Disabling console output significantly improves performance
- Increasing worker threads helps with very high log volumes
- Larger queue sizes prevent blocking when bursts of logs occur
- In memory-constrained environments, reduce queue size and increase worker threads
