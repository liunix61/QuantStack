/**
 * @file speculative_execution_engine.cpp
 * @brief 推测执行引擎实现
 * 
 * @version 1.0
 * @date 2026-08-13
 */

#include "speculative_execution_engine.hpp"
#include <iostream>

namespace quantstack {

// 初始化引擎
void SpeculativeExecutionEngine::initialize() noexcept {
    for (auto& stream : streams_) {
        stream = PreBuiltOrderStream();
    }
}

} // namespace quantstack
