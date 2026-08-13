/**
 * @file test_risk_control.cpp
 * @brief 风控系统测试
 * 
 * @version 1.0
 * @date 2026-08-13
 */

#include <gtest/gtest.h>
#include "risk_control.hpp"

using namespace quantstack;

TEST(RiskControlTest, Initialize) {
    RiskController risk_control(0.0, 1e9, 1000000, 10000000);

    EXPECT_TRUE(true);
}

TEST(RiskControlTest, CheckOrder) {
    RiskController risk_control(0.0, 1e9, 1000000, 10000000);

    Order order(1, 1001, 100.5, 1000);
    bool result = risk_control.check_order(order);

    EXPECT_TRUE(result);
}

TEST(RiskControlTest, CheckPriceRange) {
    RiskController risk_control(0.0, 1000.0, 1000000, 10000000);

    EXPECT_TRUE(risk_control.check_price_range(500.0));
    EXPECT_FALSE(risk_control.check_price_range(2000.0));
}

TEST(RiskControlTest, CheckOrderSize) {
    RiskController risk_control(0.0, 1e9, 1000, 10000000);

    EXPECT_TRUE(risk_control.check_order_size(500));
    EXPECT_FALSE(risk_control.check_order_size(2000));
}

TEST(RiskControlTest, CheckDailyLimit) {
    RiskController risk_control(0.0, 1e9, 1000000, 1000);

    Order order(1, 1001, 100.5, 100);
    EXPECT_FALSE(risk_control.check_daily_limit(1000));

    risk_control.reset_daily_limit();

    Order order2(2, 1002, 100.5, 100);
    EXPECT_TRUE(risk_control.check_daily_limit(100));
}
