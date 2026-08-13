#!/bin/bash

# QuantStack性能分析脚本
# 使用perf工具进行性能分析

set -e

echo "==================================="
echo "QuantStack 性能分析工具"
echo "==================================="

# 检查perf是否安装
if ! command -v perf &> /dev/null; then
    echo "❌ perf未安装，请先安装: sudo apt-get install linux-perf"
    exit 1
fi

# 检查可执行文件
if [ ! -f "build/quantstack_demo" ]; then
    echo "❌ 可执行文件不存在，请先编译: mkdir build && cd build && cmake .. && make"
    exit 1
fi

# 清理旧的perf数据
rm -rf perf.data perf.data.old

echo "✅ 开始性能分析..."
echo "📊 收集数据中..."

# 运行perf记录
perf record -F 99 -g -o perf.data ./build/quantstack_demo --benchmark

echo "✅ 性能分析完成！"
echo ""
echo "📊 分析选项："
echo "  1. 查看概览: perf report -i perf.data"
echo "  2. 查看Top函数: perf report -i perf.data --stdio | head -50"
echo "  3. 查看调用图: perf report -i perf.data --stdio --graph"
echo "  4. 查看热力图: perf report -i perf.data --stdio --colorgraph"
echo "  5. 查看采样: perf script -i perf.data"
echo "  6. 导出报告: perf report -i perf.data --stdio > perf_report.txt"
echo ""
echo "🔍 高级选项："
echo "  - 查看CPU缓存命中率: perf stat -e cache-references,cache-misses ./build/quantstack_demo --benchmark"
echo "  - 查看分支预测: perf stat -e branch-misses,branches ./build/quantstack_demo --benchmark"
echo "  - 查看内存带宽: perf stat -e mem-loads,mem-stores ./build/quantstack_demo --benchmark"
echo ""
echo "📁 分析结果保存在: perf.data"
