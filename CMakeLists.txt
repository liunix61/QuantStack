cmake_minimum_required(VERSION 3.20)
project(QuantStack VERSION 1.0 LANGUAGES CXX)

# 设置C++20标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -Wall -Wextra")

# 包含目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 源文件
set(SOURCES
    src/speculative_execution_engine.cpp
    src/lock_free_queue.cpp
    src/order_engine.cpp
    src/fix_protocol_layer.cpp
    src/connection_pool.cpp
    src/risk_control.cpp
    src/serializer_cache.cpp
)

# 创建可执行文件
add_executable(quantstack_demo examples/demo.cpp ${SOURCES})

# 链接库
target_link_libraries(quantstack_demo pthread)

# 测试
enable_testing()
add_subdirectory(tests)

# 安装
install(TARGETS quantstack_demo DESTINATION bin)
