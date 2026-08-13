/**
 * @file test_connection_pool.cpp
 * @brief 无锁连接池测试
 * 
 * @version 1.0
 * @date 2026-08-13
 */

#include <gtest/gtest.h>
#include "connection_pool.hpp"

using namespace quantstack;

TEST(ConnectionPoolTest, Initialization) {
    LockFreeConnectionPool pool(10);

    EXPECT_EQ(pool.size(), 10);
    EXPECT_EQ(pool.active_connections(), 0);
}

TEST(ConnectionPoolTest, AcquireReleaseConnection) {
    LockFreeConnectionPool pool(10);

    Connection* conn1 = pool.acquire_connection();
    EXPECT_NE(conn1, nullptr);

    pool.release_connection(conn1);
    pool.activate_connection(conn1);

    EXPECT_EQ(pool.active_connections(), 1);
}

TEST(ConnectionPoolTest, ResetPool) {
    LockFreeConnectionPool pool(10);

    Connection* conn1 = pool.acquire_connection();
    pool.release_connection(conn1);

    pool.reset();

    EXPECT_EQ(pool.active_connections(), 0);
}
