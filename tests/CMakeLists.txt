# 日志文件轮转测试程序的 CMake 配置
add_executable(rotation_test rotation_test.cpp)
target_link_libraries(rotation_test PRIVATE mm_logger pthread)  # 添加pthread以支持多线程
