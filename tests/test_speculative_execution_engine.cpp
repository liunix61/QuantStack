/**
 * @file test_speculative_execution_engine.cpp
 * @brief 推测执行引擎测试
 * 
 * @version 1.0
 * @date 2026-08-13
 */

#include <gtest/gtest.h>
#include "speculative_execution_engine.hpp"

using namespace quantstack;

TEST(SpeculativeExecutionEngineTest, Initialization) {
    SpeculativeExecutionEngine engine;
    engine.initialize();

    EXPECT_TRUE(true);
}

TEST(SpeculativeExecutionEngineTest, SubmitOrder) {
    SpeculativeExecutionEngine engine;
    engine.initialize();

    uint8_t packet[128];
    std::fill(packet, packet + 128, 0x00);

    // 设置Price/Qty字段
    uint64_t price = 4278560047680992256;
    uint32_t qty = 2863311530;
    std::memcpy(packet + 8, &price, 8);
    std::memcpy(packet + 16, &qty, 4);

    bool result = engine.submit_order(packet);
    EXPECT_TRUE(result);
}

TEST(SpeculativeExecutionEngineTest, ExtractPriceQty) {
    SpeculativeExecutionEngine::PriceQtyExtractor extractor;

    uint8_t packet[128];
    uint64_t expected_price = 4278560047680992256;
    uint32_t expected_qty = 2863311530;

    std::memcpy(packet + 8, &expected_price, 8);
    std::memcpy(packet + 16, &expected_qty, 4);

    uint64_t actual_price = extractor.extract_price(packet);
    uint32_t actual_qty = extractor.extract_qty(packet);

    EXPECT_EQ(actual_price, expected_price);
    EXPECT_EQ(actual_qty, expected_qty);
}
