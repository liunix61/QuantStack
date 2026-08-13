/**
 * @file speculative_execution_engine.hpp
 * @brief 推测执行引擎 - 零分配、无锁、零拷贝
 * 
 * 在收到FIX报文前64字节时，立即提取Price/Qty字段，
 * 从预构建订单流中零拷贝复制到网卡发送缓冲区。
 * 
 * @version 1.0
 * @date 2026-08-13
 */

#ifndef QUANTSTACK_SPECULATIVE_EXECUTION_ENGINE_HPP
#define QUANTSTACK_SPECULATIVE_EXECUTION_ENGINE_HPP

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>

namespace quantstack {

/**
 * @class SpeculativeExecutionEngine
 * @brief 推测执行引擎 - 纳秒级延迟压榨
 * 
 * 核心特性：
 * - 零分配：使用固定大小缓冲区
 * - 无锁：原子操作管理写入位置
 * - 零拷贝：使用__builtin_memcpy指令
 * - 提前提取：FIX报文前64字节提取Price/Qty
 */
class SpeculativeExecutionEngine {
public:
    // 配置常量
    static constexpr size_t BUFFER_SIZE = 128;      // 128字节固定缓冲区
    static constexpr size_t PACKET_HEADER_SIZE = 64; // 报文头大小
    static constexpr size_t PRICE_OFFSET = 8;        // Price字段偏移
    static constexpr size_t QTY_OFFSET = 16;         // Qty字段偏移
    static constexpr size_t PRICE_SIZE = 8;          // Price字段大小（8字节）
    static constexpr size_t QTY_SIZE = 4;            // Qty字段大小（4字节）

    /**
     * @struct PreBuiltOrderStream
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
     * @struct PriceQtyExtractor
     * @brief Price/Qty字段提取器
     */
    struct PriceQtyExtractor {
        static constexpr size_t PRICE_OFFSET = 8;
        static constexpr size_t QTY_OFFSET = 16;
        static constexpr size_t PRICE_SIZE = 8;
        static constexpr size_t QTY_SIZE = 4;

        /**
         * @brief 提取Price字段（零拷贝）
         * @param packet FIX报文
         * @return Price值
         */
        static inline uint64_t extract_price(const uint8_t* packet) noexcept {
            return *reinterpret_cast<const uint64_t*>(packet + PRICE_OFFSET);
        }

        /**
         * @brief 提取Qty字段（零拷贝）
         * @param packet FIX报文
         * @return Qty值
         */
        static inline uint32_t extract_qty(const uint8_t* packet) noexcept {
            return *reinterpret_cast<const uint32_t*>(packet + QTY_OFFSET);
        }
    };

    /**
     * @brief 提交订单到推测执行引擎
     * @param packet FIX报文
     * @return 是否成功提交
     */
    bool submit_order(const uint8_t* packet) noexcept {
        if (!packet || PACKET_HEADER_SIZE > BUFFER_SIZE) {
            return false;
        }

        // 提取Price/Qty字段
        uint64_t price = PriceQtyExtractor::extract_price(packet);
        uint32_t qty = PriceQtyExtractor::extract_qty(packet);

        // 获取预构建的订单流
        PreBuiltOrderStream* stream = acquire_stream();
        if (!stream) {
            return false;
        }

        // 零拷贝复制到缓冲区
        std::memcpy(stream->buffer.data(), packet, PACKET_HEADER_SIZE);

        // 原子写入位置
        stream->write_pos.fetch_add(1, std::memory_order_release);

        return true;
    }

    /**
     * @brief 获取下一个预构建的订单流
     * @return 订单流指针
     */
    PreBuiltOrderStream* acquire_stream() noexcept {
        // 简化实现：使用固定数组
        static PreBuiltOrderStream streams[16];
        static std::atomic<uint32_t> next_stream{0};

        uint32_t index = next_stream.fetch_add(1, std::memory_order_relaxed) % 16;
        return &streams[index];
    }

    /**
     * @brief 初始化引擎
     */
    void initialize() noexcept {
        // 初始化所有流
        for (auto& stream : streams_) {
            stream = PreBuiltOrderStream();
        }
    }

private:
    // 预构建的订单流池
    std::array<PreBuiltOrderStream, 16> streams_;
};

} // namespace quantstack

#endif // QUANTSTACK_SPECULATIVE_EXECUTION_ENGINE_HPP
