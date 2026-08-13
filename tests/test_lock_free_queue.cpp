/**
 * @file test_lock_free_queue.cpp
 * @brief 无锁队列测试
 * 
 * @version 1.0
 * @date 2026-08-13
 */

#include <gtest/gtest.h>
#include "lock_free_queue.hpp"

using namespace quantstack;

TEST(LockFreeQueueTest, EnqueueDequeue) {
    LockFreeQueue<int> queue;
    int value = 42;

    bool result = queue.enqueue(value);
    EXPECT_TRUE(result);

    int output;
    result = queue.dequeue(output);
    EXPECT_TRUE(result);
    EXPECT_EQ(output, 42);
}

TEST(LockFreeQueueTest, EmptyQueue) {
    LockFreeQueue<int> queue;
    int output;

    bool result = queue.dequeue(output);
    EXPECT_FALSE(result);
}

TEST(LockFreeQueueTest, MultipleEnqueueDequeue) {
    LockFreeQueue<int> queue;

    // 入队10个元素
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(queue.enqueue(i));
    }

    EXPECT_EQ(queue.size(), 10);

    // 出队10个元素
    for (int i = 0; i < 10; ++i) {
        int output;
        EXPECT_TRUE(queue.dequeue(output));
        EXPECT_EQ(output, i);
    }

    EXPECT_EQ(queue.size(), 0);
    EXPECT_TRUE(queue.empty());
}
