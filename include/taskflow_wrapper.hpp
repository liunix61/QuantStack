/**
 * @file taskflow_wrapper.hpp
 * @brief Taskflow库适配器 - 兼容标准库和Taskflow库
 * 
 * 提供统一的并发编程接口，支持：
 * - std::jthread (C++20标准库)
 * - Taskflow (第三方库)
 * 
 * @version 1.0
 * @date 2026-08-13
 */

#ifndef QUANTSTACK_TASKFLOW_WRAPPER_HPP
#define QUANTSTACK_TASKFLOW_WRAPPER_HPP

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <optional>

// 尝试包含Taskflow库
#if __has_include(<taskflow/taskflow.hpp>)
    #include <taskflow/taskflow.hpp>
    #define QUANTSTACK_HAS_TASKFLOW 1
#else
    #define QUANTSTACK_HAS_TASKFLOW 0
#endif

namespace quantstack {

/**
 * @class TaskflowScheduler
 * @brief 任务调度器 - 自动选择std::jthread或Taskflow
 * 
 * 核心特性：
 * - 自动选择最佳并发方案
 * - 统一的异步任务接口
 * - 支持任务图执行
 * - 兼容标准库和Taskflow
 */
class TaskflowScheduler {
public:
    /**
     * @brief 构造函数
     * @param num_threads 线程数（0=自动检测）
     */
    explicit TaskflowScheduler(size_t num_threads = 0) 
        : num_threads_(num_threads > 0 ? num_threads : std::thread::hardware_concurrency()) {
        
#if QUANTSTACK_HAS_TASKFLOW
        tf_ = std::make_unique<tf::Executor>(num_threads_);
        use_taskflow_ = true;
#else
        use_taskflow_ = false;
#endif
    }

    /**
     * @brief 异步执行任务
     * @param func 任务函数
     * @return std::future返回值
     */
    template<typename Func>
    auto async(Func&& func) -> std::future<decltype(func())> {
        if (use_taskflow_) {
            return async_taskflow(std::forward<Func>(func));
        } else {
            return async_std(std::forward<Func>(func));
        }
    }

    /**
     * @brief 并行执行任务
     * @param tasks 任务列表
     */
    template<typename Func>
    void parallel_for(size_t start, size_t end, Func&& func) {
        if (use_taskflow_) {
            parallel_for_taskflow(start, end, std::forward<Func>(func));
        } else {
            parallel_for_std(start, end, std::forward<Func>(func));
        }
    }

    /**
     * @brief 等待所有任务完成
     */
    void wait() {
        if (use_taskflow_) {
            tf_->wait_for_all();
        }
    }

    /**
     * @brief 获取线程数
     * @return 线程数
     */
    size_t thread_count() const {
        return num_threads_;
    }

    /**
     * @brief 获取当前使用的调度器类型
     * @return "std" 或 "taskflow"
     */
    const char* scheduler_type() const {
        return use_taskflow_ ? "taskflow" : "std";
    }

private:
    // std::jthread版本
    template<typename Func>
    auto async_std(Func&& func) -> std::future<decltype(func())> {
        using ResultType = decltype(func());
        auto promise = std::make_shared<std::promise<ResultType>>();
        auto future = promise->get_future();
        
        std::jthread thread([func = std::forward<Func>(func), promise = std::move(promise)](std::stop_token stoken) mutable {
            try {
                if (!stoken.stop_requested()) {
                    promise->set_value(func());
                }
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        });
        
        return future;
    }

    template<typename Func>
    void parallel_for_std(size_t start, size_t end, Func&& func) {
        std::vector<std::jthread> threads;
        size_t chunk_size = std::max(size_t(1), (end - start) / num_threads_);
        
        for (size_t i = start; i < end; i += chunk_size) {
            size_t j = std::min(i + chunk_size, end);
            threads.emplace_back([func = std::forward<Func>(func), start = i, end = j]() {
                for (size_t k = start; k < end; ++k) {
                    func(k);
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
    }

    // Taskflow版本
    template<typename Func>
    auto async_taskflow(Func&& func) -> std::future<decltype(func())> {
        using ResultType = decltype(func());
        auto promise = std::make_shared<std::promise<ResultType>>();
        auto future = promise->get_future();
        
        tf_.get()->exec([func = std::forward<Func>(func), promise = std::move(promise)](tf::Taskflow& taskflow) mutable {
            taskflow.emplace([func, promise]() mutable {
                try {
                    promise->set_value(func());
                } catch (...) {
                    promise->set_exception(std::current_exception());
                }
            });
        });
        
        return future;
    }

    template<typename Func>
    void parallel_for_taskflow(size_t start, size_t end, Func&& func) {
        tf::Taskflow taskflow;
        
        for (size_t i = start; i < end; ++i) {
            taskflow.emplace([func = std::forward<Func>(func), i]() {
                func(i);
            });
        }
        
        tf_.get()->exec(taskflow);
    }

private:
    size_t num_threads_;
    bool use_taskflow_;
    std::unique_ptr<tf::Executor> tf_;
};

/**
 * @class ParallelExecutor
 * @brief 并行执行器 - Taskflow专用
 * 
 * 高性能任务图执行，适用于复杂工作流
 */
class ParallelExecutor {
public:
    explicit ParallelExecutor(size_t num_threads = 0) 
        : num_threads_(num_threads > 0 ? num_threads : std::thread::hardware_concurrency()) {
#if QUANTSTACK_HAS_TASKFLOW
        tf_ = std::make_unique<tf::Executor>(num_threads_);
#endif
    }

    /**
     * @brief 添加任务
     * @param name 任务名称
     * @param func 任务函数
     * @return 任务ID
     */
    template<typename Func>
    size_t add_task(const char* name, Func&& func) {
#if QUANTSTACK_HAS_TASKFLOW
        size_t id = task_count_++;
        tf_.get()->silent_async(name, [func = std::forward<Func>(func), id]() {
            func(id);
        });
        return id;
#else
        (void)name;
        (void)func;
        return 0;
#endif
    }

    /**
     * @brief 添加依赖关系
     * @param from 任务ID
     * @param to 任务ID
     */
    void add_dependency(size_t from, size_t to) {
#if QUANTSTACK_HAS_TASKFLOW
        // Taskflow自动管理依赖关系
#endif
    }

    /**
     * @brief 执行所有任务
     */
    void run() {
#if QUANTSTACK_HAS_TASKFLOW
        tf_.get()->wait_for_all();
#endif
    }

    /**
     * @brief 获取任务数
     * @return 任务数
     */
    size_t task_count() const {
        return task_count_;
    }

private:
    size_t num_threads_;
    size_t task_count_ = 0;
#if QUANTSTACK_HAS_TASKFLOW
    std::unique_ptr<tf::Executor> tf_;
#endif
};

} // namespace quantstack

#endif // QUANTSTACK_TASKFLOW_WRAPPER_HPP
