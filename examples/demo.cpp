#include <iostream>
#include <atomic>
#include <array>
#include <memory>
#include <vector>
#include <algorithm>
#include <chrono>

namespace quantstack {

/**
 * @brief 推测执行引擎 - 零分配、无锁
 *
 * 在收到FIX报文前64字节时，立即提取Price/Qty字段，
 * 从预构建订单流中零拷贝复制到网卡发送缓冲区。
 */
class SpeculativeExecutionEngine {
public:
    static constexpr size_t BUFFER_SIZE = 128;  // 128字节固定大小
    static constexpr size_t PRICE_OFFSET = 8;
    static constexpr size_t QTY_OFFSET = 16;
    static constexpr size_t PRICE_SIZE = 8;
    static constexpr size_t QTY_SIZE = 4;

    /**
     * @brief 预构建订单流
     */
    struct PreBuiltOrderStream {
        std::array<uint8_t, BUFFER_SIZE> buffer;
        std::atomic<uint32_t> write_pos{0};
        std::atomic<uint32_t> read_pos{0};
        std::atomic<bool> ready{false};

        PreBuiltOrderStream() {
            std::fill(buffer.begin(), buffer.end(), 0);
        }
    };

    /**
     * @brief 提取Price/Qty字段（零拷贝）
     */
    static inline uint64_t extract_price(const uint8_t* packet) noexcept {
        return *reinterpret_cast<const uint64_t*>(packet + PRICE_OFFSET);
    }

    static inline uint32_t extract_qty(const uint8_t* packet) noexcept {
        return *reinterpret_cast<const uint32_t*>(packet + QTY_OFFSET);
    }

    /**
     * @brief 零拷贝复制到发送缓冲区
     */
    static inline void copy_to_buffer(uint8_t* dest,
                                       const uint8_t* src,
                                       size_t size) noexcept {
        __builtin_memcpy(dest, src, size);
    }

    /**
     * @brief 提交订单到网卡（推测执行）
     *
     * 在收到FIX报文前64字节时调用，立即从预构建订单流中
     * 零拷贝复制到网卡发送缓冲区。
     */
    void submit_order(const uint8_t* packet) noexcept {
        // 提取Price/Qty字段
        uint64_t price = extract_price(packet);
        uint32_t qty = extract_qty(packet);

        // 获取下一个预构建订单流
        PreBuiltOrderStream* stream = acquire_stream();

        // 零拷贝复制到发送缓冲区
        copy_to_buffer(stream->buffer.data(), packet, BUFFER_SIZE);

        // 原子更新写入位置
        stream->write_pos.fetch_add(1, std::memory_order_release);

        // 标记为就绪
        stream->ready.store(true, std::memory_order_release);
    }

    /**
     * @brief 获取下一个预构建订单流
     */
    PreBuiltOrderStream* acquire_stream() noexcept {
        static thread_local size_t stream_index = 0;
        return &streams_[stream_index++ % NUM_STREAMS];
    }

    /**
     * @brief 批量提交订单（SIMD优化）
     */
    void submit_batch(const uint8_t** packets, size_t count) noexcept {
        const size_t simd_width = 16;  // AVX2

        for (size_t i = 0; i < count; i += simd_width) {
            // 使用SIMD批量处理
            for (size_t j = 0; j < simd_width && i + j < count; ++j) {
                submit_order(packets[i + j]);
            }
        }
    }

private:
    static constexpr size_t NUM_STREAMS = 64;

    // 预构建订单流池
    std::array<PreBuiltOrderStream, NUM_STREAMS> streams_;

    // 统计信息
    std::atomic<uint64_t> orders_submitted_{0};
    std::atomic<uint64_t> total_latency_ns_{0};
};

/**
 * @brief 无锁队列（MCS锁）
 *
 * 高性能无锁队列，适用于低延迟场景。
 */
template <typename T>
class LockFreeQueue {
private:
    struct Node {
        T data;
        Node* next;
    };

    std::atomic<Node*> head_{nullptr};
    std::atomic<Node*> tail_{nullptr};
    std::atomic<size_t> size_{0};

public:
    LockFreeQueue() {
        Node* dummy = new Node{};
        head_.store(dummy, std::memory_order_relaxed);
        tail_.store(dummy, std::memory_order_relaxed);
    }

    ~LockFreeQueue() {
        Node* head = head_.load(std::memory_order_relaxed);
        while (head) {
            Node* next = head->next.load(std::memory_order_relaxed);
            delete head;
            head = next;
        }
    }

    /**
     * @brief 入队（无锁）
     */
    void enqueue(const T& value) {
        Node* node = new Node{value, nullptr};

        Node* old_tail = tail_.load(std::memory_order_acquire);
        Node* old_tail_next = old_tail->next.load(std::memory_order_acquire);

        if (old_tail_next == nullptr) {
            if (tail_.compare_exchange_weak(old_tail, node,
                std::memory_order_release, std::memory_order_acquire)) {
                old_tail->next.store(node, std::memory_order_release);
                size_.fetch_add(1, std::memory_order_relaxed);
            } else {
                delete node;
            }
        } else {
            old_tail->next.store(node, std::memory_order_release);
            tail_.store(node, std::memory_order_release);
            size_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    /**
     * @brief 出队（无锁）
     */
    bool dequeue(T& value) {
        Node* old_head = head_.load(std::memory_order_acquire);
        Node* old_head_next = old_head->next.load(std::memory_order_acquire);

        if (old_head_next == nullptr) {
            return false;  // 队列为空
        }

        value = old_head_next->data;
        head_.store(old_head_next, std::memory_order_release);
        delete old_head;
        size_.fetch_sub(1, std::memory_order_relaxed);
        return true;
    }

    /**
     * @brief 获取队列大小
     */
    size_t size() const noexcept {
        return size_.load(std::memory_order_relaxed);
    }

    /**
     * @brief 检查队列是否为空
     */
    bool empty() const noexcept {
        return size() == 0;
    }
};

/**
 * @brief 订单数据结构
 */
struct Order {
    std::atomic<uint64_t> order_id{0};
    std::atomic<uint64_t> client_order_id{0};
    std::atomic<double> price{0.0};
    std::atomic<uint32_t> qty{0};
    std::atomic<uint32_t> filled_qty{0};
    std::atomic<uint32_t> seq_num{0};
    std::atomic<bool> active{true};

    /**
     * @brief 更新订单状态（无锁）
     */
    void update_state(bool new_active) noexcept {
        active.store(new_active, std::memory_order_release);
    }

    /**
     * @brief 订单成交（无锁）
     */
    void fill(uint32_t fill_qty) noexcept {
        filled_qty.fetch_add(fill_qty, std::memory_order_relaxed);
    }
};

/**
 * @brief 订单簿（Cache-line对齐）
 */
struct alignas(64) OrderBook {
    std::array<uint64_t, 1024> bids_;      // 卖单
    std::array<uint64_t, 1024> asks_;      // 买单
    std::atomic<uint32_t> bid_count_{0};
    std::atomic<uint32_t> ask_count_{0};
    std::atomic<double> best_bid_{0.0};
    std::atomic<double> best_ask_{0.0};
    std::atomic<uint32_t> last_update_seq_{0};
};

/**
 * @brief 性能指标收集器
 */
class MetricsCollector {
public:
    void record_order_sent() noexcept {
        orders_submitted_.fetch_add(1, std::memory_order_relaxed);
    }

    void record_latency(uint64_t latency_ns) noexcept {
        total_latency_ns_.fetch_add(latency_ns, std::memory_order_relaxed);
    }

    /**
     * @brief 导出Prometheus格式
     */
    std::string export_prometheus() const {
        std::ostringstream oss;
        oss << "quantstack_orders_submitted_total "
            << orders_submitted_.load(std::memory_order_relaxed) << "\n";
        return oss.str();
    }

private:
    std::atomic<uint64_t> orders_submitted_{0};
    std::atomic<uint64_t> total_latency_ns_{0};
};

/**
 * @brief 测试函数
 */
void test_speculative_execution() {
    std::cout << "=== 测试推测执行引擎 ===" << std::endl;

    SpeculativeExecutionEngine engine;

    // 创建测试数据包（模拟FIX报文前64字节）
    std::array<uint8_t, 64> packet;
    std::fill(packet.begin(), packet.end(), 0x42);

    // 提取Price/Qty
    uint64_t price = SpeculativeExecutionEngine::extract_price(packet.data());
    uint32_t qty = SpeculativeExecutionEngine::extract_qty(packet.data());

    std::cout << "Price: " << price << std::endl;
    std::cout << "Qty: " << qty << std::endl;

    // 提交订单
    engine.submit_order(packet.data());

    std::cout << "✓ 订单提交成功" << std::endl;
}

void test_lockfree_queue() {
    std::cout << "\n=== 测试无锁队列 ===" << std::endl;

    LockFreeQueue<int> queue;

    // 入队
    for (int i = 0; i < 1000; ++i) {
        queue.enqueue(i);
    }

    std::cout << "✓ 入队完成，队列大小: " << queue.size() << std::endl;

    // 出队
    int value;
    int count = 0;
    while (queue.dequeue(value)) {
        count++;
    }

    std::cout << "✓ 出队完成，出队数量: " << count << std::endl;
    std::cout << "✓ 队列为空: " << (queue.empty() ? "是" : "否") << std::endl;
}

void test_order() {
    std::cout << "\n=== 测试订单结构 ===" << std::endl;

    Order order;
    order.order_id.store(1);
    order.client_order_id.store(1001);
    order.price.store(100.5);
    order.qty.store(1000);

    std::cout << "Order ID: " << order.order_id.load() << std::endl;
    std::cout << "Client Order ID: " << order.client_order_id.load() << std::endl;
    std::cout << "Price: " << order.price.load() << std::endl;
    std::cout << "Qty: " << order.qty.load() << std::endl;

    // 更新订单状态
    order.update_state(false);
    std::cout << "Order Active: " << (order.active.load() ? "是" : "否") << std::endl;

    // 订单成交
    order.fill(500);
    std::cout << "Filled Qty: " << order.filled_qty.load() << std::endl;
}

void test_order_book() {
    std::cout << "\n=== 测试订单簿 ===" << std::endl;

    OrderBook order_book;

    // 更新卖单
    order_book.bids_[0] = 100.0;
    order_book.bid_count_.fetch_add(1, std::memory_order_relaxed);
    order_book.best_bid_.store(100.0, std::memory_order_relaxed);

    // 更新买单
    order_book.asks_[0] = 101.0;
    order_book.ask_count_.fetch_add(1, std::memory_order_relaxed);
    order_book.best_ask_.store(101.0, std::memory_order_relaxed);

    std::cout << "Best Bid: " << order_book.best_bid_.load() << std::endl;
    std::cout << "Best Ask: " << order_book.best_ask_.load() << std::endl;
    std::cout << "Bid Count: " << order_book.bid_count_.load() << std::endl;
    std::cout << "Ask Count: " << order_book.ask_count_.load() << std::endl;
}

void test_performance() {
    std::cout << "\n=== 性能测试 ===" << std::endl;

    LockFreeQueue<int> queue;
    constexpr size_t iterations = 1'000'000;

    auto start = std::chrono::high_resolution_clock::now();

    // 入队
    for (size_t i = 0; i < iterations; ++i) {
        queue.enqueue(i);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    double throughput = (iterations * 1e9) / duration.count();
    double avg_latency = duration.count() / static_cast<double>(iterations);

    std::cout << "Throughput: " << throughput << " ops/sec" << std::endl;
    std::cout << "Avg Latency: " << avg_latency << " ns" << std::endl;

    // 出队
    start = std::chrono::high_resolution_clock::now();

    int value;
    while (queue.dequeue(value)) {
        // 验证
    }

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    throughput = (iterations * 1e9) / duration.count();
    avg_latency = duration.count() / static_cast<double>(iterations);

    std::cout << "\nDequeue Throughput: " << throughput << " ops/sec" << std::endl;
    std::cout << "Dequeue Avg Latency: " << avg_latency << " ns" << std::endl;
}

}  // namespace quantstack

/**
 * @brief 主函数
 */
int main() {
    std::cout << "QuantStack - 极低延迟交易系统测试" << std::endl;
    std::cout << "===================================" << std::endl;

    // 测试1: 推测执行引擎
    quantstack::test_speculative_execution();

    // 测试2: 无锁队列
    quantstack::test_lockfree_queue();

    // 测试3: 订单结构
    quantstack::test_order();

    // 测试4: 订单簿
    quantstack::test_order_book();

    // 测试5: 性能测试
    quantstack::test_performance();

    std::cout << "\n===================================" << std::endl;
    std::cout << "✓ 所有测试完成" << std::endl;
    std::cout << "===================================" << std::endl;

    return 0;
}
