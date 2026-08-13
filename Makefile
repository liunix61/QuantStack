# QuantStack Makefile
# 快速编译和测试

.PHONY: all clean build test install run-perf run-asan run-gprof ci

# 默认目标
all: build

# 编译
build:
	@echo "🔨 编译QuantStack..."
	@mkdir -p build
	@cd build && cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
	@cd build && ninja -j$(nproc)
	@echo "✅ 编译完成！"

# 清理
clean:
	@echo "🧹 清理构建文件..."
	@rm -rf build/
	@echo "✅ 清理完成！"

# 运行测试
test:
	@echo "🧪 运行测试..."
	@cd build && ctest --output-on-failure --verbose

# 安装
install:
	@echo "📦 安装QuantStack..."
	@cd build && cmake --install .
	@echo "✅ 安装完成！"

# 运行性能分析
run-perf:
	@./scripts/run_perf.sh

# 运行内存泄漏检测
run-asan:
	@./scripts/run_asan.sh

# 运行gprof分析
run-gprof:
	@./scripts/run_gprof.sh

# CI检查
ci: clean build test

# 帮助
help:
	@echo "QuantStack Makefile 命令："
	@echo "  make all          - 编译项目"
	@echo "  make clean        - 清理构建文件"
	@echo "  make build        - 编译项目（同all）"
	@echo "  make test         - 运行测试"
	@echo "  make install      - 安装到系统"
	@echo "  make run-perf     - 运行性能分析"
	@echo "  make run-asan     - 运行内存泄漏检测"
	@echo "  make run-gprof    - 运行gprof分析"
	@echo "  make ci           - CI检查（clean + build + test）"
	@echo "  make help         - 显示帮助信息"
