cmake_minimum_required(VERSION 3.20)
project(QuantStackDemo CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -mtune=native")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -Wall -Wextra")

# 内存检测选项（可选）
option(DEMO_ENABLE_ASAN "Enable AddressSanitizer" OFF)
if(DEMO_ENABLE_ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer")
    message(STATUS "✅ Demo: AddressSanitizer enabled")
endif()

# 性能分析选项（可选）
option(DEMO_ENABLE_GPROF "Enable gprof profiling" OFF)
if(DEMO_ENABLE_GPROF)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg -fno-omit-frame-pointer")
    message(STATUS "✅ Demo: gprof profiling enabled")
endif()

# 性能分析选项（可选）
option(DEMO_ENABLE_PERF "Enable perf profiling" OFF)
if(DEMO_ENABLE_PERF)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
    message(STATUS "✅ Demo: perf profiling enabled")
endif()

# 包含目录
include_directories(${PROJECT_SOURCE_DIR})

# 源文件
set(DEMO_SOURCES
    examples/demo.cpp
)

# 编译可执行文件
add_executable(quantstack_demo ${DEMO_SOURCES})
target_compile_options(quantstack_demo PRIVATE
    -O3
    -march=native
    -mtune=native
    -flto
    -fno-exceptions
    -fno-rtti
)

# 链接选项
target_link_libraries(quantstack_demo PRIVATE
    pthread
    dl
    taskflow
)

# 安装
install(TARGETS quantstack_demo
    RUNTIME DESTINATION bin
)

# 打印编译信息
message(STATUS "===================================")
message(STATUS "QuantStack Demo 编译配置:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  Flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "  Architecture: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  Taskflow: ${TASKFLOW_FOUND}")
message(STATUS "  AddressSanitizer: ${DEMO_ENABLE_ASAN}")
message(STATUS "  gprof: ${DEMO_ENABLE_GPROF}")
message(STATUS "  perf: ${DEMO_ENABLE_PERF}")
message(STATUS "===================================")

# 编译完成后运行测试
add_custom_command(TARGET quantstack_demo POST_BUILD
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "运行QuantStack Demo测试"
)
