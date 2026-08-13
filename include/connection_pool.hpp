/**
 * @file connection_pool.hpp
 * @brief 无锁连接池 - TCP长连接管理
 * 
 * 维护多个TCP长连接，无锁管理连接状态。
 * 
 * @version 1.0
 * @date 2026-08-13
 */

#ifndef QUANTSTACK_CONNECTION_POOL_HPP
#define QUANTSTACK_CONNECTION_POOL_HPP

#include <atomic>
#include <cstdint>
#include <cstring>
#include <vector>
#include <thread>

namespace quantstack {

/**
 * @struct Connection
 * @brief 连接结构
 */
struct Connection {
    std::atomic<uint32_t> ref_count{0};
    std::atomic<bool> active{false};
    std::atomic<int> error_count{0};
    uint32_t socket{0};
    uint8_t buffer[65536];
    std::atomic<uint32_t> buffer_pos{0};
};

/**
 * @class LockFreeConnectionPool
 * @brief 无锁连接池
 */
class LockFreeConnectionPool {
public:
    /**
     * @brief 构造函数
     * @param size 连接池大小
     */
    explicit LockFreeConnectionPool(size_t size = 16)
        : connections_(size), next_connection_(0) {
        for (auto& conn : connections_) {
            conn = Connection{};
            conn.ref_count.store(0, std::memory_order_relaxed);
            conn.active.store(false, std::memory_order_relaxed);
            conn.error_count.store(0, std::memory_order_relaxed);
            conn.buffer_pos.store(0, std::memory_order_relaxed);
        }
    }

    /**
     * @brief 获取下一个可用连接
     * @return 连接指针
     */
    Connection* acquire_connection() noexcept {
        size_t current = next_connection_.fetch_add(1, std::memory_order_relaxed);
        return &connections_[current % connections_.size()];
    }

    /**
     * @brief 释放连接
     * @param conn 连接指针
     */
    void release_connection(Connection* conn) noexcept {
        if (conn) {
            conn->ref_count.fetch_sub(1, std::memory_order_release);
        }
    }

    /**
     * @brief 激活连接
     * @param conn 连接指针
     */
    void activate_connection(Connection* conn) noexcept {
        if (conn) {
            conn->active.store(true, std::memory_order_release);
        }
    }

    /**
     * @brief 停用连接
     * @param conn 连接指针
     */
    void deactivate_connection(Connection* conn) noexcept {
        if (conn) {
            conn->active.store(false, std::memory_order_release);
        }
    }

    /**
     * @brief 获取活跃连接数
     * @return 活跃连接数
     */
    size_t active_connections() const noexcept {
        size_t count = 0;
        for (const auto& conn : connections_) {
            if (conn.active.load(std::memory_order_relaxed)) {
                ++count;
            }
        }
        return count;
    }

    /**
     * @brief 重置连接池
     */
    void reset() noexcept {
        for (auto& conn : connections_) {
            conn.ref_count.store(0, std::memory_order_relaxed);
            conn.active.store(false, std::memory_order_relaxed);
            conn.error_count.store(0, std::memory_order_relaxed);
            conn.buffer_pos.store(0, std::memory_order_relaxed);
        }
        next_connection_.store(0, std::memory_order_relaxed);
    }

    /**
     * @brief 获取连接池大小
     * @return 连接池大小
     */
    size_t size() const noexcept {
        return connections_.size();
    }

private:
    std::vector<Connection> connections_;
    std::atomic<size_t> next_connection_;
};

} // namespace quantstack

#endif // QUANTSTACK_CONNECTION_POOL_HPP
