cmake_minimum_required(VERSION 3.10)
project(mm_logger VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
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

# 设置版本信息
set(MM_LOGGER_VERSION_MAJOR 1)
set(MM_LOGGER_VERSION_MINOR 0)
set(MM_LOGGER_VERSION_PATCH 0)
set(MM_LOGGER_VERSION "${MM_LOGGER_VERSION_MAJOR}.${MM_LOGGER_VERSION_MINOR}.${MM_LOGGER_VERSION_PATCH}")

# 定义包含目录
set(MM_LOGGER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 包含CMake模块目录
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# 查找 spdlog 库，使用我们自定义的FindSpdlog.cmake模块
find_package(spdlog QUIET)

# 如果通过FindSpdlog.cmake找不到，尝试使用FetchContent下载
if(NOT spdlog_FOUND)
    include(FetchContent)
    message(STATUS "spdlog not found, downloading from GitHub...")
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.10.0  # 使用最新稳定版
    )
    # 控制spdlog行为的选项
    set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "Build spdlog examples" FORCE)
    set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "Build spdlog tests" FORCE)
    set(SPDLOG_INSTALL OFF CACHE BOOL "Generate spdlog install target" FORCE)
    FetchContent_MakeAvailable(spdlog)

    # 如果已经成功下载并且没有定义spdlog::spdlog目标，创建别名
    if(TARGET spdlog AND NOT TARGET spdlog::spdlog)
        add_library(spdlog::spdlog ALIAS spdlog)
    endif()
endif()

# 确认spdlog已找到或成功下载
if(NOT TARGET spdlog::spdlog)
    message(FATAL_ERROR "spdlog library not found and could not be downloaded!")
endif()

# 创建接口库
add_library(mm_logger INTERFACE)
target_include_directories(mm_logger INTERFACE
    $<BUILD_INTERFACE:${MM_LOGGER_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(mm_logger INTERFACE spdlog::spdlog pthread)

# 创建导出目标
install(TARGETS mm_logger
    EXPORT mm_logger-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# 安装头文件
install(DIRECTORY ${MM_LOGGER_INCLUDE_DIR}/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)

# 导出目标
install(EXPORT mm_logger-targets
    FILE mm_logger-targets.cmake
    NAMESPACE mm_logger::
    DESTINATION lib/cmake/mm_logger
)

# 创建并安装配置文件
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/mm_logger-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/mm_logger-config.cmake
    INSTALL_DESTINATION lib/cmake/mm_logger
)

# 创建版本文件
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/mm_logger-config-version.cmake
    VERSION ${MM_LOGGER_VERSION}
    COMPATIBILITY SameMajorVersion
)

# 安装配置和版本文件
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/mm_logger-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/mm_logger-config-version.cmake
    DESTINATION lib/cmake/mm_logger
)

# 可选地安装FindSpdlog.cmake以便使用mm_logger的用户能够找到spdlog
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindSpdlog.cmake
    DESTINATION lib/cmake/mm_logger
)

# 选项
option(MM_LOGGER_BUILD_TESTS "Build mm_logger tests" OFF)
option(MM_LOGGER_BUILD_EXAMPLES "Build mm_logger examples" OFF)

# 测试配置
if(MM_LOGGER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 示例配置
if(MM_LOGGER_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# 打印配置信息
message(STATUS "mm_logger version: ${MM_LOGGER_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Build tests: ${MM_LOGGER_BUILD_TESTS}")
message(STATUS "Build examples: ${MM_LOGGER_BUILD_EXAMPLES}")
