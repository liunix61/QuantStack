# QuantStack - 极低延迟交易系统

## 📌 项目概述

**QuantStack** 是一个**极致低延迟交易系统**，专为币安VIP 9级别的高频交易场景设计。系统采用**推测执行架构**，在网卡接收到完整FIX协议报文之前，提前几十个字节就触发订单发送，实现纳秒级延迟压榨。

## 🎯 核心目标

- **延迟**: <100纳秒（P99）
- **吞吐**: >1M订单/秒
- **可靠性**: 99.9999%
- **分配**: 零分配（Zero-Allocation）

## 🏗️ 系统架构

```
硬件层
├── DPDK网卡（XDP旁路）
├── CPU Cache（L1/L2/L3）
└── 内存控制器（NUMA优化）

内核旁路层
├── DPDK User Space Ring Buffer
├── 推测执行引擎
└── XDP Program（eBPF）

应用层
├── FIX协议层
├── 订单引擎
├── 连接池
├── 状态机
└── 风控系统

数据层
├── 内存池
├── 对象池
└── 序列化缓存
```

## 📦 核心模块

### 1. 推测执行引擎（Speculative Execution Engine）

- 零分配：固定大小缓冲区
- 无锁：原子操作管理
- 零拷贝：SIMD指令优化
- 提前提取：FIX报文前64字节提取Price/Qty

### 2. 无锁队列（Lock-Free Queue）

- 基于MCS锁
- 原子操作管理队列状态
- 支持多线程并发

### 3. 订单引擎（Order Engine）

- 订单生命周期管理
- 无锁状态更新
- 状态机支持

### 4. FIX协议层（FIX Protocol Layer）

- 二进制序列化/反序列化
- SIMD指令优化
- 校验和计算

### 5. 无锁连接池（Lock-Free Connection Pool）

- TCP长连接管理
- 无锁连接状态
- 错误处理和重连

### 6. 风控系统（Risk Control System）

- 价格范围检查
- 数量限制检查
- 每日限额检查

### 7. 内存池（Memory Pool）

- SLAB分配器
- 对象池
- 零分配

### 8. 序列化缓存（Serializer Cache）

- 零拷贝操作
- SIMD指令优化
- 高性能序列化

## 🛠️ 技术栈

| 技术 | 用途 |
|------|------|
| **C++20** | 核心语言 |
| **DPDK + XDP** | 内核旁路 |
| **FIX 4.5/5.0** | 协议 |
| **SIMD** | AVX2优化 |
| **无锁编程** | 并发控制 |

## 🚀 快速开始

### 编译

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 运行示例

```bash
./quantstack_demo
```

### 运行测试

```bash
cd tests
mkdir build && cd build
cmake ..
make
ctest --output-on-failure
```

## 📖 文档

- [系统架构文档](../QuantStack-System-Architecture.md)
- [快速参考](../QuantStack-Quick-Reference.md)
- [项目结构](../QuantStack-Project-Structure.md)

## 📝 许可证

MIT License

## 📧 联系方式

- GitHub: https://github.com/liunix61/QuantStack
- Email: your.email@example.com

---

**版本**: 1.0
**最后更新**: 2026-08-13
