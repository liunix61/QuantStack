/**
 * @file order_engine.hpp
 * @brief 订单引擎 - 订单生命周期管理
 * 
 * 处理订单生命周期，从订单创建到发送到市场。
 * 
 * @version 1.0
 * @date 2026-08-13
 */

#ifndef QUANTSTACK_ORDER_ENGINE_HPP
#define QUANTSTACK_ORDER_ENGINE_HPP

#include <atomic>
#include <cstdint>
#include <string>

namespace quantstack {

/**
 * @enum OrderState
 * @brief 订单状态
 */
enum class OrderState {
    NEW,              // 新订单
    PARTIALLY_FILLED, // 部分成交
    FILLED,           // 完全成交
    CANCELLED,        // 已取消
    REJECTED          // 已拒绝
};

/**
 * @brief 将OrderState转换为字符串
 * @param state 订单状态
 * @return 状态字符串
 */
inline const char* to_string(OrderState state) {
    switch (state) {
        case OrderState::NEW: return "NEW";
        case OrderState::PARTIALLY_FILLED: return "PARTIALLY_FILLED";
        case OrderState::FILLED: return "FILLED";
        case OrderState::CANCELLED: return "CANCELLED";
        case OrderState::REJECTED: return "REJECTED";
        default: return "UNKNOWN";
    }
}

/**
 * @class Order
 * @brief 订单结构
 */
class Order {
public:
    /**
     * @brief 构造函数
     * @param order_id 订单ID
     * @param client_order_id 客户订单ID
     * @param price 价格
     * @param qty 数量
     */
    Order(uint64_t order_id = 0,
          uint64_t client_order_id = 0,
          double price = 0.0,
          uint32_t qty = 0)
        : order_id_(order_id),
          client_order_id_(client_order_id),
          price_(price),
          qty_(qty),
          filled_qty_(0),
          seq_num_(0) {}

    /**
     * @brief 获取订单ID
     * @return 订单ID
     */
    uint64_t order_id() const { return order_id_.load(std::memory_order_relaxed); }

    /**
     * @brief 获取客户订单ID
     * @return 客户订单ID
     */
    uint64_t client_order_id() const { return client_order_id_.load(std::memory_order_relaxed); }

    /**
     * @brief 获取价格
     * @return 价格
     */
    double price() const { return price_.load(std::memory_order_relaxed); }

    /**
     * @brief 获取数量
     * @return 数量
     */
    uint32_t qty() const { return qty_.load(std::memory_order_relaxed); }

    /**
     * @brief 获取已成交数量
     * @return 已成交数量
     */
    uint32_t filled_qty() const { return filled_qty_.load(std::memory_order_relaxed); }

    /**
     * @brief 获取订单状态
     * @return 订单状态
     */
    OrderState state() const { return state_.load(std::memory_order_relaxed); }

    /**
     * @brief 获取序列号
     * @return 序列号
     */
    uint32_t seq_num() const { return seq_num_.load(std::memory_order_relaxed); }

    /**
     * @brief 更新订单状态
     * @param new_state 新状态
     */
    void update_state(OrderState new_state) noexcept {
        state_.store(new_state, std::memory_order_release);
    }

    /**
     * @brief 更新已成交数量
     * @param fill_qty 成交数量
     */
    void fill(uint32_t fill_qty) noexcept {
        filled_qty_.fetch_add(fill_qty, std::memory_order_relaxed);
    }

    /**
     * @brief 设置序列号
     * @param seq_num 序列号
     */
    void set_seq_num(uint32_t seq_num) noexcept {
        seq_num_.store(seq_num, std::memory_order_relaxed);
    }

private:
    std::atomic<OrderState> state_;
    std::atomic<uint64_t> order_id_;
    std::atomic<uint64_t> client_order_id_;
    std::atomic<double> price_;
    std::atomic<uint32_t> qty_;
    std::atomic<uint32_t> filled_qty_;
    std::atomic<uint32_t> seq_num_;
};

} // namespace quantstack

#endif // QUANTSTACK_ORDER_ENGINE_HPP
