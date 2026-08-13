/**
 * @file test_fix_protocol_layer.cpp
 * @brief FIX协议层测试
 * 
 * @version 1.0
 * @date 2026-08-13
 */

#include <gtest/gtest.h>
#include "fix_protocol_layer.hpp"

using namespace quantstack;

TEST(FIXProtocolLayerTest, SerializeOrder) {
    Order order(1, 1001, 100.5, 1000);

    uint8_t buffer[256];
    size_t size = BinaryFIXSerializer::serialize_order(order, buffer);

    EXPECT_EQ(size, 24);
    EXPECT_NE(buffer[0], 0);
}

TEST(FIXProtocolLayerTest, CalculateChecksum) {
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    uint16_t checksum = BinaryFIXSerializer::calculate_checksum(data, 4);

    EXPECT_NE(checksum, 0);
}

TEST(FIXProtocolLayerTest, ZeroCopySerializer) {
    uint64_t value = 4278560047680992256;

    uint8_t buffer[16];
    BinaryFIXSerializer::serialize_int(value, buffer);

    uint64_t result = BinaryFIXSerializer::deserialize_int(buffer);

    EXPECT_EQ(result, value);
}

TEST(FIXProtocolLayerTest, FloatSerialization) {
    double value = 100.5;

    uint8_t buffer[16];
    BinaryFIXSerializer::serialize_float(value, buffer);

    double result = BinaryFIXSerializer::deserialize_float(buffer);

    EXPECT_DOUBLE_EQ(result, 100.5);
}
