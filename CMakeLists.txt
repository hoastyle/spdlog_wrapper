cmake_minimum_required(VERSION 3.10)
project(mm_logger)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置构建类型，如果没有指定的话
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# 查找 spdlog 库
# 方法1: 使用 find_package (如果 spdlog 已安装到系统)
find_package(spdlog QUIET)

# 方法2: 如果找不到系统安装的 spdlog，可以使用 FetchContent 下载
if(NOT spdlog_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.5.0  # 使用特定版本 1.5.0
    )
    FetchContent_MakeAvailable(spdlog)
endif()

# 方法3: 或者可以使用子模块
# 如果使用子模块，取消下面的注释
# add_subdirectory(external/spdlog)

# 添加头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 添加库文件
add_library(mm_logger INTERFACE)
target_include_directories(mm_logger INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(mm_logger INTERFACE spdlog::spdlog)

# # 创建示例可执行文件
# add_executable(logger_example examples/example.cpp)
# target_link_libraries(logger_example mm_logger)

# 安装规则
install(DIRECTORY include/ DESTINATION include)

# 测试配置
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 示例配置
option(BUILD_EXAMPLES "Build examples" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
