/**
 * @file fix_protocol_layer.hpp
 * @brief FIX协议层 - 二进制序列化/反序列化
 * 
 * 处理币安VIP 9专属的FIX 4.5/5.0协议，二进制序列化/反序列化。
 * 
 * @version 1.0
 * @date 2026-08-13
 */

#ifndef QUANTSTACK_FIX_PROTOCOL_LAYER_HPP
#define QUANTSTACK_FIX_PROTOCOL_LAYER_HPP

#include <cstdint>
#include <cstring>
#include <string>

namespace quantstack {

/**
 * @brief FIX报文类型
 */
enum class FIXMessageType {
    NEW_ORDER_SINGLE = 0x01,
    ORDER_CANCEL_REQUEST = 0x02,
    ORDER_CANCEL_REPLACE_REQUEST = 0x03,
    MARKET_DATA_REQUEST = 0x04,
    ORDER_EXECUTED = 0x05,
    ORDER_FILL = 0x06,
    MARKET_DATA = 0x07
};

/**
 * @class BinaryFIXSerializer
 * @brief FIX二进制序列化器
 */
class BinaryFIXSerializer {
public:
    /**
     * @brief 序列化订单报文
     * @param order 订单
     * @param buffer 输出缓冲区
     * @return 序列化后的长度
     */
    static size_t serialize_order(const Order& order, uint8_t* buffer) noexcept {
        if (!buffer) {
            return 0;
        }

        // 报文头（8字节）
        uint32_t message_type = static_cast<uint32_t>(FIXMessageType::NEW_ORDER_SINGLE);
        uint32_t seq_num = order.seq_num();
        
        // 使用SIMD指令加速序列化
        __m128i message_type_vec = _mm_set1_epi32(message_type);
        __m128i seq_num_vec = _mm_set1_epi32(seq_num);
        __m128i price_vec = _mm_set1_epi64x(static_cast<uint64_t>(order.price() * 100000000));
        __m128i qty_vec = _mm_set1_epi32(order.qty());

        // 零拷贝复制到缓冲区
        std::memcpy(buffer, &message_type, 4);
        std::memcpy(buffer + 4, &seq_num, 4);
        std::memcpy(buffer + 8, &price_vec, 8);
        std::memcpy(buffer + 16, &qty_vec, 4);

        return 24; // 24字节
    }

    /**
     * @brief 反序列化市场数据
     * @param buffer 输入缓冲区
     * @param market_data 市场数据输出
     * @return 反序列化后的长度
     */
    static size_t deserialize_market_data(const uint8_t* buffer, MarketData& market_data) noexcept {
        if (!buffer) {
            return 0;
        }

        // 使用SIMD指令加速反序列化
        __m128i temp;
        std::memcpy(&temp, buffer, 8);
        market_data.price = _mm_cvtsi128_si32(temp);

        std::memcpy(&temp, buffer + 8, 8);
        market_data.bid_price = _mm_cvtsi128_si32(temp);
        market_data.bid_qty = *reinterpret_cast<const uint32_t*>(buffer + 16);
        market_data.ask_price = *reinterpret_cast<const uint32_t*>(buffer + 20);
        market_data.ask_qty = *reinterpret_cast<const uint32_t*>(buffer + 24);

        return 32; // 32字节
    }

    /**
     * @brief 计算校验和
     * @param buffer 输入缓冲区
     * @param length 长度
     * @return 校验和
     */
    static uint16_t calculate_checksum(const uint8_t* buffer, size_t length) noexcept {
        uint16_t checksum = 0;

        for (size_t i = 0; i < length; ++i) {
            checksum += buffer[i];
        }

        return checksum;
    }

private:
    /**
     * @struct MarketData
     * @brief 市场数据结构
     */
    struct MarketData {
        uint64_t price;
        uint32_t bid_price;
        uint32_t bid_qty;
        uint32_t ask_price;
        uint32_t ask_qty;
    };
};

} // namespace quantstack

#endif // QUANTSTACK_FIX_PROTOCOL_LAYER_HPP
