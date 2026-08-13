cmake_minimum_required(VERSION 3.20)
project(QuantStackTests VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${PROJECT_SOURCE_DIR}/include)

# 测试库
add_library(quantstack_lib STATIC
    src/speculative_execution_engine.cpp
    src/lock_free_queue.cpp
    src/order_engine.cpp
    src/fix_protocol_layer.cpp
    src/connection_pool.cpp
    src/risk_control.cpp
    src/serializer_cache.cpp
)

target_link_libraries(quantstack_lib pthread)

# Catch2测试框架
include(FetchContent)
FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.3.2
)
FetchContent_MakeAvailable(catch2)

# 添加测试
add_executable(tests
    test_speculative_execution_engine.cpp
    test_lock_free_queue.cpp
    test_order_engine.cpp
    test_fix_protocol_layer.cpp
    test_connection_pool.cpp
    test_risk_control.cpp
)

target_link_libraries(tests quantstack_lib catch2::catch2)

# CTest配置
enable_testing()
add_test(NAME AllTests COMMAND tests)
