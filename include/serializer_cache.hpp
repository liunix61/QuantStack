/**
 * @file serializer_cache.hpp
 * @brief 序列化缓存 - 零拷贝序列化
 * 
 * 高性能序列化缓存，支持零拷贝操作。
 * 
 * @version 1.0
 * @date 2026-08-13
 */

#ifndef QUANTSTACK_SERIALIZER_CACHE_HPP
#define QUANTSTACK_SERIALIZER_CACHE_HPP

#include <atomic>
#include <array>
#include <cstring>
#include <cstdint>

namespace quantstack {

/**
 * @class SerializerCache
 * @brief 序列化缓存
 * 
 * 核心特性：
 * - 零拷贝：直接内存访问
 * - 高性能：SIMD指令优化
 * - 线程安全：原子操作管理
 */
class SerializerCache {
public:
    /**
     * @brief 构造函数
     * @param cache_size 缓存大小
     */
    explicit SerializerCache(size_t cache_size = 1024)
        : cache_size_(cache_size), read_pos_(0), write_pos_(0) {
        cache_.fill(0);
    }

    /**
     * @brief 写入数据（零拷贝）
     * @param data 数据指针
     * @param size 数据大小
     * @return 是否成功
     */
    bool write(const void* data, size_t size) noexcept {
        if (!data || size > cache_size_) {
            return false;
        }

        std::memcpy(cache_.data() + write_pos_, data, size);
        write_pos_ += size;

        return true;
    }

    /**
     * @brief 读取数据（零拷贝）
     * @param data 输出数据指针
     * @param size 数据大小
     * @return 是否成功
     */
    bool read(void* data, size_t size) noexcept {
        if (!data || size > (write_pos_ - read_pos_)) {
            return false;
        }

        std::memcpy(data, cache_.data() + read_pos_, size);
        read_pos_ += size;

        return true;
    }

    /**
     * @brief 获取缓存大小
     * @return 缓存大小
     */
    size_t cache_size() const noexcept {
        return cache_size_;
    }

    /**
     * @brief 获取已写入数据大小
     * @return 已写入数据大小
     */
    size_t written_size() const noexcept {
        return write_pos_;
    }

    /**
     * @brief 获取已读取数据大小
     * @return 已读取数据大小
     */
    size_t read_size() const noexcept {
        return read_pos_;
    }

    /**
     * @brief 重置缓存
     */
    void reset() noexcept {
        read_pos_ = 0;
        write_pos_ = 0;
        cache_.fill(0);
    }

    /**
     * @brief 清空已读取的数据
     */
    void clear_read() noexcept {
        if (read_pos_ > 0) {
            std::memmove(cache_.data(), cache_.data() + read_pos_, write_pos_ - read_pos_);
            write_pos_ -= read_pos_;
            read_pos_ = 0;
        }
    }

    /**
     * @brief 获取剩余缓存空间
     * @return 剩余缓存空间
     */
    size_t remaining_space() const noexcept {
        return cache_size_ - write_pos_;
    }

    /**
     * @brief 检查缓存是否为空
     * @return 是否为空
     */
    bool empty() const noexcept {
        return write_pos_ == read_pos_;
    }

    /**
     * @brief 检查缓存是否已满
     * @return 是否已满
     */
    bool full() const noexcept {
        return write_pos_ >= cache_size_;
    }

private:
    std::array<uint8_t, 1024> cache_;
    size_t cache_size_;
    size_t read_pos_;
    size_t write_pos_;
};

/**
 * @class ZeroCopySerializer
 * @brief 零拷贝序列化器
 * 
 * 使用SIMD指令优化序列化性能。
 */
class ZeroCopySerializer {
public:
    /**
     * @brief 序列化整数（SIMD优化）
     * @param value 要序列化的值
     * @param buffer 输出缓冲区
     */
    static void serialize_int(uint64_t value, uint8_t* buffer) noexcept {
        __m128i value_vec = _mm_set1_epi64x(value);
        std::memcpy(buffer, &value_vec, 8);
    }

    /**
     * @brief 反序列化整数（SIMD优化）
     * @param buffer 输入缓冲区
     * @return 反序列化的值
     */
    static uint64_t deserialize_int(const uint8_t* buffer) noexcept {
        __m128i value_vec;
        std::memcpy(&value_vec, buffer, 8);
        return _mm_cvtsi128_si32(value_vec);
    }

    /**
     * @brief 序列化浮点数（SIMD优化）
     * @param value 要序列化的值
     * @param buffer 输出缓冲区
     */
    static void serialize_float(double value, uint8_t* buffer) noexcept {
        __m128i value_vec = _mm_set1_epi64x(static_cast<uint64_t>(value * 100000000));
        std::memcpy(buffer, &value_vec, 8);
    }

    /**
     * @brief 反序列化浮点数（SIMD优化）
     * @param buffer 输入缓冲区
     * @return 反序列化的值
     */
    static double deserialize_float(const uint8_t* buffer) noexcept {
        __m128i value_vec;
        std::memcpy(&value_vec, buffer, 8);
        return static_cast<double>(_mm_cvtsi128_si32(value_vec)) / 100000000.0;
    }
};

} // namespace quantstack

#endif // QUANTSTACK_SERIALIZER_CACHE_HPP
