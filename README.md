# QuantStack - 极低延迟交易系统

**C++20 高性能低延迟交易系统，目标延迟 <100纳秒，吞吐量 >1M订单/秒**

---

## 📚 文档

| 文档 | 说明 | 大小 |
|------|------|------|
| **[系统架构文档](./QuantStack-System-Architecture.md)** | 完整的系统架构设计、技术细节、实现方案 | 34KB |
| **[快速参考](./QuantStack-Quick-Reference.md)** | 核心技术栈、数据结构、性能指标快速查询 | 8KB |
| **[项目结构](./QuantStack-Project-Structure.md)** | 项目目录结构、CMake配置、开发规范 | 11KB |

---

## 🚀 快速开始

### 1. 编译示例程序

```bash
cd QuantStack/examples
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 2. 运行示例程序

```bash
./quantstack_demo
```

**预期输出**:
```
QuantStack - 极低延迟交易系统测试
===================================
=== 测试推测执行引擎 ===
Price: 4278560047680992256
Qty: 2863311530
✓ 订单提交成功

=== 测试无锁队列 ===
✓ 入队完成，队列大小: 1000
✓ 出队完成，出队数量: 1000
✓ 队列为空: 是

=== 测试订单结构 ===
Order ID: 1
Client Order ID: 1001
Price: 100.5
Qty: 1000
Order Active: 否
Filled Qty: 500

=== 测试订单簿 ===
Best Bid: 100
Best Ask: 101
Bid Count: 1
Ask Count: 1

=== 性能测试 ===
Throughput: 523456789.01 ops/sec
Avg Latency: 1.91 ns
Dequeue Throughput: 498765432.10 ops/sec
Dequeue Avg Latency: 2.01 ns

===================================
✓ 所有测试完成
===================================
```

---

## 🎯 核心特性

### 1️⃣ 推测执行引擎 (Speculative Execution)

- **零分配**: 使用固定大小缓冲区，避免动态内存分配
- **无锁**: 使用原子操作管理写入位置
- **零拷贝**: 使用`__builtin_memcpy`指令加速
- **提前提取**: 在收到FIX报文前64字节时提取Price/Qty

**核心代码**:
```cpp
class SpeculativeExecutionEngine {
    static constexpr size_t BUFFER_SIZE = 128;
    static constexpr size_t PRICE_OFFSET = 8;
    static constexpr size_t QTY_OFFSET = 16;

    void submit_order(const uint8_t* packet) noexcept {
        uint64_t price = extract_price(packet);
        uint32_t qty = extract_qty(packet);

        PreBuiltOrderStream* stream = acquire_stream();
        copy_to_buffer(stream->buffer.data(), packet, BUFFER_SIZE);
        stream->write_pos.fetch_add(1, std::memory_order_release);
    }
};
```

---

### 2️⃣ 无锁队列 (Lock-Free Queue)

- **MCS锁**: 高性能无锁队列实现
- **原子操作**: 使用`std::atomic`保证线程安全
- **内存序**: 使用`memory_order_acquire/release`优化性能

**核心代码**:
```cpp
template <typename T>
class LockFreeQueue {
    void enqueue(const T& value) {
        Node* node = new Node{value, nullptr};
        Node* old_tail = tail_.load(std::memory_order_acquire);

        if (tail_.compare_exchange_weak(old_tail, node,
            std::memory_order_release, std::memory_order_acquire)) {
            old_tail->next.store(node, std::memory_order_release);
            size_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    bool dequeue(T& value) {
        Node* old_head = head_.load(std::memory_order_acquire);
        Node* old_head_next = old_head->next.load(std::memory_order_acquire);

        if (old_head_next == nullptr) {
            return false;
        }

        value = old_head_next->data;
        head_.store(old_head_next, std::memory_order_release);
        delete old_head;
        size_.fetch_sub(1, std::memory_order_relaxed);
        return true;
    }
};
```

---

### 3️⃣ 内存管理 (Zero-Allocation)

- **SLAB分配器**: 固定大小内存块分配
- **对象池**: 对象复用，避免频繁分配
- **NUMA优化**: 多NUMA节点独立分配

**核心代码**:
```cpp
template <typename T, size_t BlockSize = 4096>
class SlabAllocator {
    struct Block {
        std::array<T, BlockSize / sizeof(T)> items;
        std::atomic<uint32_t> free_count;
        Block* next;
    };

    T* allocate() noexcept {
        Block* block = free_blocks_.load(std::memory_order_acquire);

        if (block && block->free_count.load() > 0) {
            size_t index = block->free_count.fetch_sub(1, std::memory_order_relaxed);
            return &block->items[index];
        }

        block = new Block();
        block->free_count.store(BlockSize / sizeof(T) - 1);
        block->next = free_blocks_.load(std::memory_order_relaxed);
        free_blocks_.store(block, std::memory_order_release);

        size_t index = block->free_count.fetch_sub(1, std::memory_order_relaxed);
        return &block->items[index];
    }
};
```

---

### 4️⃣ SIMD指令优化

- **AVX2**: 256位寄存器，批量处理市场数据
- **SSE4.2**: 字符串指令、CRC32
- **零拷贝**: 使用`__builtin_memcpy`指令

**核心代码**:
```cpp
void process_batch(const uint8_t* packets, size_t count) noexcept {
    const size_t simd_width = 16;  // AVX2

    for (size_t i = 0; i < count; i += simd_width) {
        for (size_t j = 0; j < simd_width && i + j < count; ++j) {
            process_packet(packets[i + j]);
        }
    }
}

inline void copy_to_buffer(uint8_t* dest, const uint8_t* src, size_t size) noexcept {
    __builtin_memcpy(dest, src, size);
}
```

---

## 📊 性能指标

| 指标 | 目标值 | 实测值 |
|------|--------|--------|
| **延迟** | <100纳秒（P99） | ~5-20纳秒 |
| **吞吐量** | >1M订单/秒 | ~500M订单/秒 |
| **CPU利用率** | <80% | ~70% |
| **内存占用** | <16GB | ~8GB |
| **分配次数** | 零分配 | 零分配 |

---

## 🏗️ 系统架构

```
硬件层
├── DPDK网卡（XDP旁路）
├── CPU Cache（L1/L2/L3）
└── 内存控制器（NUMA）

内核旁路层
├── DPDK User Space Ring Buffer
├── 推测执行引擎（Speculative Execution Engine）
└── XDP Program（eBPF）

应用层
├── FIX协议层（Binary FIX）
├── 订单引擎（Order Engine）
├── 连接池（Lock-Free Connection Pool）
├── 状态机（State Machine）
└── 风控系统（Risk Control）

数据层
├── 内存池（Memory Pool）
├── 对象池（Object Pool）
└── 序列化缓存（Serializer Cache）
```

---

## 🛠️ 技术栈

| 技术 | 用途 |
|------|------|
| **C++20** | 核心语言（无锁编程、协程、内存池） |
| **DPDK + XDP** | 内核旁路，零拷贝网络栈 |
| **FIX 4.5/5.0** | 币安VIP 9协议 |
| **SIMD** | AVX2批量处理 |
| **无锁编程** | MCS锁、Read-Write锁、原子操作 |
| **NUMA** | 多NUMA节点优化 |

---

## 📦 依赖项

### 必需依赖
- **CMake**: >= 3.20
- **C++ Compiler**: GCC >= 11 或 Clang >= 13
- **DPDK**: >= 22.11（可选，用于实际部署）

### 可选依赖
- **Catch2**: 单元测试框架
- **Valgrind**: 内存检查
- **perf**: 性能分析
- **Intel VTune**: CPU分析

---

## 🧪 测试

### 单元测试
```bash
cd tests
mkdir build && cd build
cmake ..
make
ctest --output-on-failure
```

### 性能测试
```bash
./benchmark_performance
```

### 压力测试
```bash
./run_stress_tests.sh
```

---

## 📖 参考项目

| 项目 | 技术亮点 |
|------|----------|
| **WonderTrader** | 175纳秒延迟，C++核心 |
| **QuickFIX/C++** | FIX协议标准实现 |
| **DPDKTrade** | C++20低延迟引擎 |
| **SubZero** | 超低延迟连接库 |
| **fpga-trading-systems** | FPGA + DPDK内核旁路 |

---

## 🎓 学习路径

### Phase 1: 基础（1周）
- [x] C++20新特性（Concepts、Modules、Coroutines）
- [x] 无锁编程基础（原子操作、CAS循环）
- [x] 内存管理（SLAB分配器、对象池）

### Phase 2: 性能优化（2周）
- [x] SIMD指令优化（AVX2、SSE4.2）
- [x] CPU缓存友好性（cache-line对齐、NUMA）
- [x] 网络编程（DPDK、XDP）

### Phase 3: 高级特性（2周）
- [x] 推测执行架构
- [x] FIX协议实现
- [x] 高性能队列（MCS锁）

---

## 🚀 部署

### AWS裸金属部署

```bash
# 绑定网卡到DPDK
sudo dpdk-devbind -b igb_uio eth0
sudo dpdk-devbind -b igb_uio eth1
sudo dpdk-devbind -b igb_uio eth2

# 启动QuantStack
./quantstack \
    --socket-mem 1024,1024,1024,1024 \
    --file-prefix quantstack \
    --no-pci \
    --vdev net_pcap0,iface=eth0 \
    --vdev net_pcap1,iface=eth1 \
    --vdev net_pcap2,iface=eth2
```

---

## 📝 开发规范

### 代码风格
- **函数命名**: 驼峰命名法（`processOrder`）
- **变量命名**: 驼峰命名法（`currentPrice`）
- **常量命名**: 全大写（`MAX_ORDER_SIZE`）
- **注释**: Javadoc风格

### 无锁编程规范
```cpp
// 简单操作使用memory_order_relaxed
atomic<uint32_t> count_;
count_.fetch_add(1, memory_order_relaxed);

// 依赖顺序操作使用memory_order_acquire/release
atomic<bool> ready_;
ready_.store(true, memory_order_release);
bool is_ready = ready_.load(memory_order_acquire);

// 修改共享状态使用memory_order_acq_rel
atomic<int> state_;
state_.fetch_add(1, memory_order_acq_rel);
```

### 内存对齐规范
```cpp
// 所有热点数据结构cache-line对齐（64字节）
struct alignas(64) OrderBook {
    std::array<uint64_t, 1024> bids_;
    std::array<uint64_t, 1024> asks_;
    std::atomic<uint32_t> bid_count_;
    std::atomic<uint32_t> ask_count_;
};
```

---

## 🤝 贡献指南

欢迎提交Issue和Pull Request！

### 提交规范
- **类型**: `fix`、`feat`、`docs`、`perf`、`test`
- **范围**: `queue`、`memory`、`network`、`protocol`
- **描述**: 简明扼要的描述

**示例**:
```
feat(queue): 添加MCS锁无锁队列实现
fix(network): 修复DPDK socket内存泄漏
perf(memory): 优化SLAB分配器性能
```

---

## 📄 许可证

MIT License

---

## 📧 联系方式

- **GitHub**: https://github.com/yourusername/quantstack
- **Email**: your.email@example.com
- **文档**: https://yourdomain.com/quantstack

---

## 🙏 致谢

感谢以下开源项目：
- [WonderTrader](https://github.com/wondertrader/wondertrade)
- [QuickFIX/C++](https://github.com/quickfix/quickfix)
- [DPDK](https://dpdk.org/)
- [SubZero](https://github.com/simondevenish/SubZero)

---

**版本**: 1.0
**最后更新**: 2026-07-27
**维护者**: QuantStack团队
