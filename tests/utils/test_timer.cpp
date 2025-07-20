#include <gtest/gtest.h>
#include <gmock/gmock.h> // 使用 ::testing::ElementsAre 等匹配器需要此头文件
#include <iostream>
#include <chrono>
#include <atomic>
#include <thread>
#include <vector>
#include <mutex>
#include <memory>

#include "../../src/utils/timer.hpp"

// 使用测试固件进行设置和清理
class TimerTest : public ::testing::Test {
protected:
    // unique_ptr 确保定时器的内存被自动管理。
    std::unique_ptr<utils::Timer> timer;

    // SetUp() 会在该固件的每个测试开始前被调用。
    void SetUp() override {
        // 为每个测试创建一个新的 Timer 实例，以保证测试间的隔离性。
        timer = std::make_unique<utils::Timer>();
    }

    // TearDown() 会在每个测试结束后被调用。
    void TearDown() override {
        // 显式停止定时器以清理状态。
        if (timer) {
            timer->stop();
        }
    }
};

// 测试一个单次任务能够被正确执行且仅执行一次。
TEST_F(TimerTest, OnceTaskExecutesCorrectly) {
    std::atomic<int> counter{0};
    auto start_time = std::chrono::steady_clock::now();

    // 添加一个 100ms 后执行一次的任务
    timer->addOnceTask(std::chrono::milliseconds(100), [&counter]() {
        counter++;
    });

    timer->start();

    // 等待稍长于 100ms 的时间，确保任务有足够时间执行
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    timer->stop(); // 停止计时器，以便对最终状态进行断言

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    EXPECT_EQ(counter.load(), 1);
    EXPECT_GE(duration.count(), 100);
}

// 测试周期性任务会执行多次。
TEST_F(TimerTest, PeriodicTaskExecutesMultipleTimes) {
    std::atomic<int> counter{0};

    // 添加一个周期性任务：50ms 初始延迟，每 100ms 重复一次
    timer->addPeriodicTask(
        std::chrono::milliseconds(50),  // 初始延迟
        std::chrono::milliseconds(100), // 周期
        [&counter]() {
            counter++;
        }
    );

    timer->start();

    // 等待 380ms。任务预计在 ~50ms, ~150ms, ~250ms, ~350ms 时执行。
    // 稍长的等待时间使测试更加健壮。
    std::this_thread::sleep_for(std::chrono::milliseconds(380));
    timer->stop();

    int final_count = counter.load();
    // 考虑到时间精度问题，执行次数可能是 3 或 4。
    EXPECT_GE(final_count, 3);
    EXPECT_LE(final_count, 4);
}

// 测试同时处理多个不同类型的任务。
TEST_F(TimerTest, HandlesMultipleDifferentTasks) {
    std::atomic<int> task1_count{0};
    std::atomic<int> task2_count{0};
    std::atomic<int> task3_count{0};

    // 添加多个任务
    timer->addOnceTask(std::chrono::milliseconds(50), [&task1_count]() { task1_count++; });
    timer->addOnceTask(std::chrono::milliseconds(100), [&task2_count]() { task2_count++; });
    timer->addPeriodicTask(
        std::chrono::milliseconds(25), // 初始延迟
        std::chrono::milliseconds(75),  // 周期
        [&task3_count]() { task3_count++; }
    );

    timer->start();

    // 等待 220ms.
    // 任务1 在 ~50ms 执行。
    // 任务2 在 ~100ms 执行。
    // 任务3 在 ~25ms, ~100ms, ~175ms 执行 (3次)。
    std::this_thread::sleep_for(std::chrono::milliseconds(220));
    timer->stop();

    EXPECT_EQ(task1_count.load(), 1);
    EXPECT_EQ(task2_count.load(), 1);
    EXPECT_GE(task3_count.load(), 2); // 应该至少执行2次（很可能是3次）。
}

// 测试任务按照其设定的延迟顺序执行。
TEST_F(TimerTest, TasksExecuteInCorrectOrder) {
    std::vector<int> execution_order;
    std::mutex order_mutex;

    // 添加不同延迟的任务以验证执行顺序
    timer->addOnceTask(std::chrono::milliseconds(150), [&]() {
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(3);
    });
    timer->addOnceTask(std::chrono::milliseconds(75), [&]() {
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(2);
    });
    timer->addOnceTask(std::chrono::milliseconds(25), [&]() {
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(1);
    });

    timer->start();

    // 等待足够长的时间以确保所有任务都已完成
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    timer->stop();

    // 使用 gmock 匹配器可以简洁且全面地检查 vector 的内容和顺序。
    EXPECT_THAT(execution_order, ::testing::ElementsAre(1, 2, 3));
}

// 测试定时器可以被停止和重启，并继续执行任务。
TEST_F(TimerTest, StopAndRestartResumesExecution) {
    std::atomic<int> counter{0};

    // 添加一个每 50ms 执行一次的周期性任务
    timer->addPeriodicTask(
        std::chrono::milliseconds(50),
        std::chrono::milliseconds(50),
        [&counter]() { counter++; }
    );

    timer->start();
    // 等待 120ms, 预期执行 2 次 (在 ~50ms 和 ~100ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    timer->stop();

    int first_count = counter.load();
    EXPECT_GE(first_count, 1); // 应该至少执行了1次。

    // 短暂等待，确保工作线程完全停止。
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 重启定时器
    timer->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    timer->stop();

    int second_count = counter.load();
    // 重启后的计数值应该大于停止前的计数值。
    EXPECT_GT(second_count, first_count);
}

// gtest 的主入口点。
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}