/**
 * @file test_order_engine.cpp
 * @brief 订单引擎测试
 * 
 * @version 1.0
 * @date 2026-08-13
 */

#include <gtest/gtest.h>
#include "order_engine.hpp"

using namespace quantstack;

TEST(OrderEngineTest, OrderCreation) {
    Order order(1, 1001, 100.5, 1000);

    EXPECT_EQ(order.order_id(), 1);
    EXPECT_EQ(order.client_order_id(), 1001);
    EXPECT_DOUBLE_EQ(order.price(), 100.5);
    EXPECT_EQ(order.qty(), 1000);
}

TEST(OrderEngineTest, OrderState) {
    Order order(1, 1001, 100.5, 1000);

    EXPECT_EQ(order.state(), OrderState::NEW);

    order.update_state(OrderState::PARTIALLY_FILLED);
    EXPECT_EQ(order.state(), OrderState::PARTIALLY_FILLED);

    order.update_state(OrderState::FILLED);
    EXPECT_EQ(order.state(), OrderState::FILLED);
}

TEST(OrderEngineTest, OrderFill) {
    Order order(1, 1001, 100.5, 1000);

    EXPECT_EQ(order.filled_qty(), 0);

    order.fill(500);
    EXPECT_EQ(order.filled_qty(), 500);

    order.fill(300);
    EXPECT_EQ(order.filled_qty(), 800);
}

TEST(OrderEngineTest, OrderStateToString) {
    EXPECT_STREQ(to_string(OrderState::NEW), "NEW");
    EXPECT_STREQ(to_string(OrderState::PARTIALLY_FILLED), "PARTIALLY_FILLED");
    EXPECT_STREQ(to_string(OrderState::FILLED), "FILLED");
    EXPECT_STREQ(to_string(OrderState::CANCELLED), "CANCELLED");
    EXPECT_STREQ(to_string(OrderState::REJECTED), "REJECTED");
}
