cmake_minimum_required(VERSION 3.10)
project(mm_logger_perf_test VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 查找依赖项
find_package(Threads REQUIRED)
find_package(spdlog REQUIRED)

# 定义源文件
set(PERF_TEST_SOURCES
    src/main.cpp
    src/perf_config.cpp
    src/perf_test.cpp
    src/test_suites.cpp
    src/utils.cpp
)

# 创建可执行文件
add_executable(performance_test ${PERF_TEST_SOURCES})

# 设置包含目录
target_include_directories(performance_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 链接依赖库
target_link_libraries(performance_test PRIVATE
    mm_logger
    spdlog::spdlog
    Threads::Threads
)

# 设置输出目录
set_target_properties(performance_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# 安装支持脚本
install(FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_performance_tests.sh"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/analyze_perf_results.py"
    DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

# 复制脚本到构建目录
add_custom_command(TARGET performance_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_performance_tests.sh"
    "${CMAKE_BINARY_DIR}/run_performance_tests.sh"
)

add_custom_command(TARGET performance_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/analyze_perf_results.py"
    "${CMAKE_BINARY_DIR}/analyze_perf_results.py"
)

add_custom_command(TARGET performance_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/perf_monitor.py"
    "${CMAKE_BINARY_DIR}/perf_monitor.py"
)

# 显示构建完成消息
add_custom_command(TARGET performance_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Built performance test: performance_test"
    COMMAND ${CMAKE_COMMAND} -E echo "Run performance tests with: ./run_performance_tests.sh"
)
