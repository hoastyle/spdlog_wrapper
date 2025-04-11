# examples/CMakeLists.txt

# 定义所有示例程序
set(EXAMPLE_SOURCES
    example.cpp
    example_multithread.cpp
    # 将来可以在这里添加更多示例文件
)

# 自动为每个示例文件创建可执行文件
foreach(example_source ${EXAMPLE_SOURCES})
    # 从源文件名提取可执行文件名
    get_filename_component(example_name ${example_source} NAME_WE)

    # 创建可执行文件
    add_executable(${example_name} ${example_source})

    # 链接mm_logger库
    target_link_libraries(${example_name} PRIVATE mm_logger pthread)

    # 设置输出目录
    set_target_properties(${example_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
    )

    # 添加自定义消息，在每个示例构建完成后显示
    add_custom_command(TARGET ${example_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Built example: ${example_name}"
    )
endforeach()

# 告诉用户如何运行多线程测试
message(STATUS "Multi-threaded example can be run with: ./examples/example_multithread [options]")
message(STATUS "Use --help to see available options")
