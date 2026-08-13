#!/bin/bash

# QuantStack内存泄漏检测脚本
# 使用AddressSanitizer检测内存泄漏

set -e

echo "==================================="
echo "QuantStack 内存泄漏检测"
echo "==================================="

# 检查可执行文件
if [ ! -f "build/quantstack_demo" ]; then
    echo "❌ 可执行文件不存在，请先编译: mkdir build && cd build && cmake -DQUANTSTACK_ENABLE_ASAN=ON .. && make"
    exit 1
fi

echo "✅ 开始内存泄漏检测..."
echo "🔍 使用AddressSanitizer..."

# 运行测试
cd build
ASAN_OPTIONS=detect_leaks=1:halt_on_error=0:print_summary=1:log_path=/tmp/asan_log ./quantstack_demo

echo ""
echo "✅ 检测完成！"
echo ""
echo "📊 分析选项："
echo "  1. 查看总结: cat /tmp/asan_log.* | grep 'SUMMARY'"
echo "  2. 查看详细日志: cat /tmp/asan_log.*"
echo "  3. 使用valgrind对比: valgrind --leak-check=full ./quantstack_demo"
echo ""
echo "💡 建议："
echo "  - 启用ASan编译: cmake -DQUANTSTACK_ENABLE_ASAN=ON .."
echo "  - 使用valgrind进行更详细分析: valgrind --leak-check=full --show-leak-kinds=all ./build/quantstack_demo"
echo ""
echo "📁 日志保存在: /tmp/asan_log.*"
