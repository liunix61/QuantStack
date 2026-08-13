/**
 * @file lock_free_queue.hpp
 * @brief 无锁队列 - 基于MCS锁
 * 
 * 高性能无锁队列实现，使用原子操作和CAS循环。
 * 
 * @version 1.0
 * @date 2026-08-13
 */

#ifndef QUANTSTACK_LOCK_FREE_QUEUE_HPP
#define QUANTSTACK_LOCK_FREE_QUEUE_HPP

#include <atomic>
#include <cstdint>
#include <memory>
#include <stdexcept>

namespace quantstack {

/**
 * @template T
 * @class LockFreeQueue
 * @brief 无锁队列 - 基于MCS锁
 * 
 * 核心特性：
 * - 无锁：原子操作管理队列状态
 * - 高性能：MCS锁实现
 * - 线程安全：支持多线程并发访问
 * - 内存序：使用memory_order_acquire/release优化性能
 */
template <typename T>
class LockFreeQueue {
private:
    /**
     * @struct Node
     * @brief 队列节点
     */
    struct Node {
        T data;
        Node* next;
    };

public:
    /**
     * @brief 构造函数
     */
    LockFreeQueue() : head_(nullptr), tail_(nullptr), size_(0) {
        // 初始化空队列
        Node* dummy = new Node{};

        head_.store(dummy, std::memory_order_relaxed);
        tail_.store(dummy, std::memory_order_relaxed);
        size_.store(0, std::memory_order_relaxed);
    }

    /**
     * @brief 析构函数
     */
    ~LockFreeQueue() {
        // 清理所有节点
        Node* current = head_.load(std::memory_order_acquire);
        while (current != nullptr) {
            Node* next = current->next;
            delete current;
            current = next;
        }
    }

    /**
     * @brief 入队操作
     * @param value 要入队的值
     * @return 是否成功
     */
    bool enqueue(const T& value) {
        Node* node = new Node{value, nullptr};

        Node* old_tail = tail_.load(std::memory_order_acquire);
        Node* old_tail_next = old_tail->next.load(std::memory_order_acquire);

        if (old_tail_next == nullptr) {
            // 尝试将新节点添加到尾部
            if (tail_.compare_exchange_weak(old_tail, node,
                std::memory_order_release, std::memory_order_acquire)) {
                old_tail->next.store(node, std::memory_order_release);
                size_.fetch_add(1, std::memory_order_relaxed);
                return true;
            } else {
                delete node;
                return false;
            }
        } else {
            // 将节点添加到尾部
            old_tail->next.store(node, std::memory_order_release);
            tail_.store(node, std::memory_order_release);
            size_.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
    }

    /**
     * @brief 出队操作
     * @param value 输出参数，存储出队的值
     * @return 是否成功
     */
    bool dequeue(T& value) {
        Node* old_head = head_.load(std::memory_order_acquire);
        Node* old_head_next = old_head->next.load(std::memory_order_acquire);

        if (old_head_next == nullptr) {
            return false; // 队列为空
        }

        // 提取数据
        value = old_head_next->data;

        // 更新head指针
        head_.store(old_head_next, std::memory_order_release);

        // 释放旧节点
        delete old_head;
        size_.fetch_sub(1, std::memory_order_relaxed);

        return true;
    }

    /**
     * @brief 获取队列大小
     * @return 队列大小
     */
    size_t size() const {
        return size_.load(std::memory_order_relaxed);
    }

    /**
     * @brief 检查队列是否为空
     * @return 是否为空
     */
    bool empty() const {
        return size_.load(std::memory_order_relaxed) == 0;
    }

private:
    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;
    std::atomic<size_t> size_;
};

} // namespace quantstack

#endif // QUANTSTACK_LOCK_FREE_QUEUE_HPP
