cmake_minimum_required(VERSION 3.20)
project(QuantStack VERSION 1.0 LANGUAGES CXX)

# 设置C++20标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O2 -g -Wall -Wextra")

# 内存检测选项（可按需启用）
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer")

# 性能分析选项（可按需启用）
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg -fno-omit-frame-pointer")
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg -fno-omit-frame-pointer")

# 包含目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# Taskflow库配置
include(FetchContent)
FetchContent_Declare(
    taskflow
    GIT_REPOSITORY https://github.com/taskflow/taskflow.git
    GIT_TAG v3.7.0
)
FetchContent_MakeAvailable(taskflow)
set(TASKFLOW_FOUND TRUE)

# 源文件
set(SOURCES
    src/speculative_execution_engine.cpp
    src/lock_free_queue.cpp
    src/order_engine.cpp
    src/fix_protocol_layer.cpp
    src/connection_pool.cpp
    src/risk_control.cpp
    src/serializer_cache.cpp
    src/memory_pool.cpp
)

# 创建可执行文件
add_executable(quantstack_demo examples/demo.cpp ${SOURCES})

# 链接库
target_link_libraries(quantstack_demo pthread dl taskflow)

# 测试
enable_testing()
add_subdirectory(tests)

# 安装
install(TARGETS quantstack_demo DESTINATION bin)

# 编译信息
message(STATUS "===================================")
message(STATUS "QuantStack 编译配置:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  Flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "  Architecture: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  Taskflow: ${TASKFLOW_FOUND}")
message(STATUS "===================================")
