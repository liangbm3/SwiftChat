#include <gtest/gtest.h>
#include "../../src/utils/thread_pool.hpp"
#include <iostream>
#include <chrono>
#include <atomic>
#include <vector>
#include <future>

using namespace utils;

// 测试辅助函数
namespace {
    // 测试函数1：简单的计算任务
    int add(int a, int b) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return a + b;
    }

    // 测试函数2：无返回值的任务
    void print_message(const std::string &msg) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        // 注意：在测试中避免直接输出到 cout
    }

    // 测试函数3：计算密集型任务
    long long fibonacci(int n) {
        if (n <= 1)
            return n;
        return fibonacci(n - 1) + fibonacci(n - 2);
    }
}

// ThreadPool 测试类
class ThreadPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 测试开始前的设置
    }

    void TearDown() override {
        // 测试结束后的清理
    }
};

// 测试1：基本功能测试
TEST_F(ThreadPoolTest, BasicFunctionality) {
    ThreadPool pool(4);

    // 提交有返回值的任务
    auto result1 = pool.enqueue(add, 10, 20);
    auto result2 = pool.enqueue(add, 5, 15);

    // 等待结果并验证
    EXPECT_EQ(result1.get(), 30);
    EXPECT_EQ(result2.get(), 20);
}

// 测试2：并发任务测试
TEST_F(ThreadPoolTest, ConcurrentTasks) {
    ThreadPool pool(4);
    std::vector<std::future<void>> futures;
    std::atomic<int> task_count{0};

    // 提交多个并发任务
    for (int i = 0; i < 10; ++i) {
        futures.push_back(
            pool.enqueue([&task_count, i]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                task_count++;
            }));
    }

    // 等待所有任务完成
    for (auto &future : futures) {
        future.get();
    }

    EXPECT_EQ(task_count.load(), 10);
}

// 测试3：性能测试
TEST_F(ThreadPoolTest, Performance) {
    const int num_tasks = 20;
    ThreadPool pool(4);

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::future<long long>> futures;
    for (int i = 0; i < num_tasks; ++i) {
        futures.push_back(pool.enqueue(fibonacci, 30));
    }

    // 等待所有任务完成
    long long total = 0;
    for (auto &future : futures) {
        total += future.get();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // 验证结果正确性（fibonacci(30) = 832040）
    EXPECT_EQ(total, 832040LL * num_tasks);
    
    // 性能断言：并发执行应该比串行快
    // 单线程执行20次fibonacci(30)大约需要更长时间
    EXPECT_LT(duration.count(), 10000); // 应该在10秒内完成
}

// 测试4：异常处理测试
TEST_F(ThreadPoolTest, ExceptionHandling) {
    ThreadPool pool(2);

    // 提交一个会抛出异常的任务
    auto future = pool.enqueue([]() -> int {
        throw std::runtime_error("测试异常");
        return 42;
    });

    // 验证异常能够被正确捕获
    EXPECT_THROW({
        try {
            future.get();
        } catch (const std::runtime_error& e) {
            EXPECT_STREQ(e.what(), "测试异常");
            throw;
        }
    }, std::runtime_error);

    // 验证线程池仍然可以处理正常任务
    auto normal_future = pool.enqueue([]() { return 42; });
    EXPECT_EQ(normal_future.get(), 42);
}

// 测试5：线程池销毁测试
TEST_F(ThreadPoolTest, ThreadPoolDestruction) {
    // 在作用域内创建线程池
    {
        ThreadPool pool(2);
        
        // 提交一些任务
        std::vector<std::future<int>> futures;
        for (int i = 0; i < 10; ++i) {
            futures.push_back(pool.enqueue([i]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                return i * i;
            }));
        }
        
        // 等待任务完成
        for (int i = 0; i < 10; ++i) {
            EXPECT_EQ(futures[i].get(), i * i);
        }
    } // 线程池在这里被销毁
    
    // 如果程序没有崩溃或挂起，说明销毁正常
    SUCCEED();
}

// 测试6：边界条件测试  
TEST_F(ThreadPoolTest, EdgeCases) {
    // 测试单线程池
    {
        ThreadPool pool(1);
        auto future = pool.enqueue([]() { return 42; });
        EXPECT_EQ(future.get(), 42);
    }
    
    // 测试提交任务到已销毁的线程池（通过作用域控制）
    // 注意：实际使用中应避免这种情况
    std::future<int> future;
    {
        ThreadPool pool(2);
        // 在线程池还存在时提交任务
        future = pool.enqueue([]() { 
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return 100; 
        });
        // 这里线程池即将被销毁，但任务应该已经在执行
    }
    
    // 等待任务完成（即使线程池已被销毁，任务仍应完成）
    EXPECT_EQ(future.get(), 100);
}

// 测试7：停止后的任务提交测试
TEST_F(ThreadPoolTest, EnqueueAfterStop) {
    // 注意：ThreadPool类当前没有显式的stop方法
    // 这个测试主要验证线程池销毁后的行为
    
    std::shared_ptr<ThreadPool> pool_ptr = std::make_shared<ThreadPool>(2);
    
    // 提交一个正常任务
    auto future1 = pool_ptr->enqueue([]() { return 42; });
    EXPECT_EQ(future1.get(), 42);
    
    // 销毁线程池
    pool_ptr.reset();
    
    // 在实际应用中，应该避免在线程池销毁后继续使用
    // 这里只是测试程序的健壮性
    SUCCEED(); // 如果程序没有崩溃，测试通过
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
