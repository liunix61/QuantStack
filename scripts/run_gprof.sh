#!/bin/bash

# QuantStack gprof性能分析脚本
# 使用gprof工具进行性能分析

set -e

echo "==================================="
echo "QuantStack gprof性能分析"
echo "==================================="

# 检查可执行文件
if [ ! -f "build/quantstack_demo" ]; then
    echo "❌ 可执行文件不存在，请先编译: mkdir build && cd build && cmake -DQUANTSTACK_ENABLE_GPROF=ON .. && make"
    exit 1
fi

# 检查gprof是否安装
if ! command -v gprof &> /dev/null; then
    echo "❌ gprof未安装，请先安装: sudo apt-get install gprof"
    exit 1
fi

echo "✅ 开始gprof性能分析..."
echo "📊 收集性能数据..."

# 运行程序并生成gprof数据
cd build
./quantstack_demo --benchmark > /tmp/gprof_input.txt

echo "✅ 数据收集完成！"
echo ""

# 生成gprof报告
gprof quantstack_demo /tmp/gprof_data > /tmp/gprof_report.txt

echo "✅ 性能分析完成！"
echo ""
echo "📊 分析选项："
echo "  1. 查看Top函数: cat /tmp/gprof_report.txt | grep -A 20 'Flat profile'"
echo "  2. 查看调用图: cat /tmp/gprof_report.txt | grep -A 20 '"%"'
echo "  3. 导出报告: gprof quantstack_demo /tmp/gprof_data > gprof_report.txt"
echo ""
echo "💡 建议："
echo "  - 启用gprof编译: cmake -DQUANTSTACK_ENABLE_GPROF=ON .."
echo "  - 使用perf进行更现代的分析: ./scripts/run_perf.sh"
echo ""
echo "📁 分析结果保存在: /tmp/gprof_report.txt"
