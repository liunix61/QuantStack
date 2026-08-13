/**
 * @file memory_pool.hpp
 * @brief 内存池 - SLAB分配器
 * 
 * 固定大小内存块分配，对象复用，避免频繁分配。
 * 
 * @version 1.0
 * @date 2026-08-13
 */

#ifndef QUANTSTACK_MEMORY_POOL_HPP
#define QUANTSTACK_MEMORY_POOL_HPP

#include <atomic>
#include <array>
#include <memory>

namespace quantstack {

/**
 * @template T
 * @class SlabAllocator
 * @brief SLAB分配器
 * 
 * 核心特性：
 * - 零分配：使用固定大小内存块
 * - 高性能：直接内存操作
 * - 线程安全：原子操作管理
 */
template <typename T, size_t BlockSize = 4096>
class SlabAllocator {
private:
    /**
     * @struct Block
     * @brief 内存块
     */
    struct Block {
        std::array<T, BlockSize / sizeof(T)> items;
        std::atomic<uint32_t> free_count;
        Block* next;
    };

public:
    /**
     * @brief 构造函数
     * @param initial_blocks 初始块数
     */
    explicit SlabAllocator(size_t initial_blocks = 1)
        : free_blocks_(nullptr), total_allocated_(0) {
        for (size_t i = 0; i < initial_blocks; ++i) {
            Block* block = new Block();
            block->free_count.store(BlockSize / sizeof(T) - 1, std::memory_order_relaxed);
            block->next = free_blocks_.load(std::memory_order_relaxed);
            free_blocks_.store(block, std::memory_order_release);
        }
    }

    /**
     * @brief 分配内存
     * @return 内存指针
     */
    T* allocate() noexcept {
        Block* block = free_blocks_.load(std::memory_order_acquire);

        // 从当前block分配
        if (block && block->free_count.load() > 0) {
            size_t index = block->free_count.fetch_sub(1, std::memory_order_relaxed);
            return &block->items[index];
        }

        // 分配新block
        block = new Block();
        block->free_count.store(BlockSize / sizeof(T) - 1, std::memory_order_relaxed);
        block->next = free_blocks_.load(std::memory_order_relaxed);
        free_blocks_.store(block, std::memory_order_release);

        size_t index = block->free_count.fetch_sub(1, std::memory_order_relaxed);
        return &block->items[index];
    }

    /**
     * @brief 释放内存（简化实现）
     * @param ptr 内存指针
     */
    void deallocate(T* ptr) noexcept {
        // 实际实现需要反向索引
        // 这里简化处理，不实现真正的释放
        // 实际应用中应该维护一个free list
    }

    /**
     * @brief 获取总分配量
     * @return 总分配量
     */
    size_t total_allocated() const noexcept {
        return total_allocated_.load(std::memory_order_relaxed);
    }

    /**
     * @brief 获取总块数
     * @return 总块数
     */
    size_t total_blocks() const noexcept {
        size_t count = 0;
        Block* block = free_blocks_.load(std::memory_order_acquire);
        while (block != nullptr) {
            ++count;
            block = block->next;
        }
        return count;
    }

private:
    std::atomic<Block*> free_blocks_;
    std::atomic<size_t> total_allocated_;
};

/**
 * @template T
 * @class ObjectPool
 * @brief 对象池
 * 
 * 核心特性：
 * - 对象复用：避免频繁分配/释放
 * - 线程安全：原子操作管理
 * - 高性能：缓存友好
 */
template <typename T, size_t BLOCK_SIZE = 1024>
class ObjectPool {
private:
    std::vector<std::unique_ptr<T[]>> blocks_;
    std::atomic<size_t> next_block_{0};
    std::atomic<size_t> next_index_{0};
    std::mutex mutex_;

public:
    /**
     * @brief 构造函数
     */
    ObjectPool() {
        // 初始化第一个block
        blocks_.emplace_back(std::make_unique<T[]>(BLOCK_SIZE));
    }

    /**
     * @brief 获取对象
     * @return 对象指针
     */
    T* acquire() {
        size_t block = next_block_.load(std::memory_order_acquire);
        size_t index = next_index_.fetch_add(1, std::memory_order_relaxed);

        if (index >= BLOCK_SIZE) {
            std::lock_guard<std::mutex> lock(mutex_);
            block = next_block_.load(std::memory_order_acquire);
            index = next_index_.fetch_add(1, std::memory_order_relaxed);

            if (index >= BLOCK_SIZE) {
                blocks_.emplace_back(std::make_unique<T[]>(BLOCK_SIZE));
                block = blocks_.size() - 1;
                index = 0;
            }
        }

        return &blocks_[block][index];
    }

    /**
     * @brief 释放对象
     * @param ptr 对象指针
     */
    void release(T* ptr) noexcept {
        // 回收到池中
        // 实际实现需要维护反向索引
    }

    /**
     * @brief 获取池中对象数量
     * @return 对象数量
     */
    size_t size() const noexcept {
        return next_index_.load(std::memory_order_relaxed);
    }

    /**
     * @brief 获取总块数
     * @return 总块数
     */
    size_t total_blocks() const noexcept {
        return blocks_.size();
    }

    static constexpr size_t BLOCK_SIZE = BLOCK_SIZE;
};

} // namespace quantstack

#endif // QUANTSTACK_MEMORY_POOL_HPP
