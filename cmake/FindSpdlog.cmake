# FindSpdlog.cmake - 查找spdlog库
#
# 该模块定义了以下变量：
#  SPDLOG_FOUND        - 如果找到spdlog库则为true
#  SPDLOG_INCLUDE_DIRS - spdlog头文件所在的目录
#  SPDLOG_LIBRARIES    - spdlog库的链接信息
#
# 此外，还定义了以下导入目标：
#  spdlog::spdlog      - 导入目标，用于链接

# 检查是否已经找到spdlog（可能是由其他方式添加的，如子模块）
if(TARGET spdlog::spdlog)
  set(SPDLOG_FOUND TRUE)
  return()
endif()

# 尝试查找spdlog的包配置文件（由spdlog安装时提供）
find_package(spdlog CONFIG QUIET)
if(spdlog_FOUND)
  # 包配置文件方式，应该已经创建了spdlog::spdlog目标
  set(SPDLOG_FOUND TRUE)
  if(NOT TARGET spdlog::spdlog)
    # 如果没有创建目标，创建别名目标兼容性
    if(TARGET spdlog)
      add_library(spdlog::spdlog ALIAS spdlog)
    endif()
  endif()
  return()
endif()

# 如果包配置方式失败，尝试通过路径查找

# 查找头文件路径
find_path(SPDLOG_INCLUDE_DIR
  NAMES spdlog/spdlog.h
  PATHS
    ${CMAKE_INSTALL_PREFIX}/include
    /usr/include
    /usr/local/include
    /opt/local/include
  DOC "spdlog include directory"
)

# spdlog可以是仅头文件库或带静态/动态库
# 首先查找库文件
find_library(SPDLOG_LIBRARY
  NAMES spdlog spdlogd
  PATHS
    ${CMAKE_INSTALL_PREFIX}/lib
    ${CMAKE_INSTALL_PREFIX}/lib64
    /usr/lib
    /usr/lib64
    /usr/local/lib
    /usr/local/lib64
    /opt/local/lib
  DOC "spdlog library"
)

# 处理查找结果
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(spdlog
  REQUIRED_VARS SPDLOG_INCLUDE_DIR
  VERSION_VAR SPDLOG_VERSION
)

# 如果找到库，设置变量
if(SPDLOG_FOUND)
  set(SPDLOG_INCLUDE_DIRS ${SPDLOG_INCLUDE_DIR})

  # 如果找到库文件，添加到库列表
  if(SPDLOG_LIBRARY)
    set(SPDLOG_LIBRARIES ${SPDLOG_LIBRARY})
    set(SPDLOG_COMPILE_DEFINITIONS SPDLOG_COMPILED_LIB)
  else()
    # 仅头文件模式
    set(SPDLOG_LIBRARIES "")
    set(SPDLOG_COMPILE_DEFINITIONS "")
  endif()

  # 创建导入目标
  if(NOT TARGET spdlog::spdlog)
    add_library(spdlog::spdlog INTERFACE IMPORTED)
    set_target_properties(spdlog::spdlog PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${SPDLOG_INCLUDE_DIRS}"
    )

    if(SPDLOG_LIBRARY)
      set_target_properties(spdlog::spdlog PROPERTIES
        INTERFACE_LINK_LIBRARIES "${SPDLOG_LIBRARIES}"
        INTERFACE_COMPILE_DEFINITIONS "${SPDLOG_COMPILE_DEFINITIONS}"
      )
    endif()
  endif()
endif()

# 隐藏高级变量
mark_as_advanced(
  SPDLOG_INCLUDE_DIR
  SPDLOG_LIBRARY
)
