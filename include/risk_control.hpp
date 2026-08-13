/**
 * @file risk_control.hpp
 * @brief 风控系统 - 订单风险监控
 * 
 * 实时监控订单风险，防止异常交易。
 * 
 * @version 1.0
 * @date 2026-08-13
 */

#ifndef QUANTSTACK_RISK_CONTROL_HPP
#define QUANTSTACK_RISK_CONTROL_HPP

#include <atomic>
#include <cstdint>
#include <string>

namespace quantstack {

/**
 * @class RiskController
 * @brief 风控检查器
 */
class RiskController {
public:
    /**
     * @brief 构造函数
     * @param min_price 最小价格
     * @param max_price 最大价格
     * @param max_order_size 最大订单数量
     * @param daily_limit 每日限额
     */
    RiskController(double min_price = 0.0,
                   double max_price = 1e9,
                   uint32_t max_order_size = 1000000,
                   uint32_t daily_limit = 10000000)
        : min_price_(min_price),
          max_price_(max_price),
          max_order_size_(max_order_size),
          daily_limit_(daily_limit),
          daily_volume_(0) {}

    /**
     * @brief 检查订单是否合规
     * @param order 订单
     * @return 是否合规
     */
    bool check_order(const Order& order) noexcept {
        // 检查价格范围
        if (order.price() < min_price_ || order.price() > max_price_) {
            return false;
        }

        // 检查数量限制
        if (order.qty() > max_order_size_) {
            return false;
        }

        // 检查每日限额
        uint32_t new_volume = daily_volume_.fetch_add(order.qty(), std::memory_order_relaxed);
        if (new_volume > daily_limit_) {
            daily_volume_.fetch_sub(order.qty(), std::memory_order_relaxed);
            return false;
        }

        return true;
    }

    /**
     * @brief 检查价格范围
     * @param price 价格
     * @return 是否在范围内
     */
    bool check_price_range(double price) noexcept {
        return price >= min_price_ && price <= max_price_;
    }

    /**
     * @brief 检查订单数量
     * @param qty 数量
     * @return 是否在限制内
     */
    bool check_order_size(uint32_t qty) noexcept {
        return qty <= max_order_size_;
    }

    /**
     * @brief 检查每日限额
     * @param qty 数量
     * @return 是否超过限额
     */
    bool check_daily_limit(uint32_t qty) noexcept {
        uint32_t new_volume = daily_volume_.fetch_add(qty, std::memory_order_relaxed);
        if (new_volume > daily_limit_) {
            daily_volume_.fetch_sub(qty, std::memory_order_relaxed);
            return true; // 超过限额
        }
        return false;
    }

    /**
     * @brief 重置每日限额
     */
    void reset_daily_limit() noexcept {
        daily_volume_.store(0, std::memory_order_relaxed);
    }

    /**
     * @brief 设置价格范围
     * @param min_price 最小价格
     * @param max_price 最大价格
     */
    void set_price_range(double min_price, double max_price) noexcept {
        min_price_ = min_price;
        max_price_ = max_price;
    }

    /**
     * @brief 设置订单数量限制
     * @param max_order_size 最大订单数量
     */
    void set_max_order_size(uint32_t max_order_size) noexcept {
        max_order_size_ = max_order_size;
    }

    /**
     * @brief 设置每日限额
     * @param daily_limit 每日限额
     */
    void set_daily_limit(uint32_t daily_limit) noexcept {
        daily_limit_ = daily_limit;
    }

    /**
     * @brief 获取每日交易量
     * @return 每日交易量
     */
    uint32_t get_daily_volume() const noexcept {
        return daily_volume_.load(std::memory_order_relaxed);
    }

    /**
     * @brief 获取剩余限额
     * @return 剩余限额
     */
    uint32_t get_remaining_limit() const noexcept {
        uint32_t volume = daily_volume_.load(std::memory_order_relaxed);
        return daily_limit_ - volume;
    }

private:
    std::atomic<double> min_price_;
    std::atomic<double> max_price_;
    std::atomic<uint32_t> max_order_size_;
    std::atomic<uint32_t> daily_limit_;
    std::atomic<uint32_t> daily_volume_;
};

} // namespace quantstack

#endif // QUANTSTACK_RISK_CONTROL_HPP
