#include <iostream>
#include <cassert>
#include <chrono>
#include <atomic>
#include <thread>
#include "../../src/utils/timer.hpp"

class TimerTest {
private:
    utils::Timer timer;
    
public:
    void testOnceTask() {
        std::cout << "Testing once task..." << std::endl;
        
        std::atomic<int> counter{0};
        auto start_time = std::chrono::steady_clock::now();
        
        // 添加一个100ms后执行的任务
        timer.addOnceTask(std::chrono::milliseconds(100), [&counter]() {
            counter++;
        });
        
        timer.start();
        
        // 等待150ms确保任务执行
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        assert(counter == 1);
        assert(duration >= std::chrono::milliseconds(100));
        
        timer.stop();
        std::cout << "Once task test passed! Duration: " << duration.count() << "ms" << std::endl;
    }
    
    void testPeriodicTask() {
        std::cout << "Testing periodic task..." << std::endl;
        
        std::atomic<int> counter{0};
        auto start_time = std::chrono::steady_clock::now();
        
        // 添加一个50ms延迟，每100ms执行一次的周期性任务
        timer.addPeriodicTask(
            std::chrono::milliseconds(50),   // 初始延迟
            std::chrono::milliseconds(100),  // 周期
            [&counter]() {
                counter++;
            }
        );
        
        timer.start();
        
        // 等待350ms，应该执行3次（50ms + 100ms + 100ms + 100ms = 350ms）
        std::this_thread::sleep_for(std::chrono::milliseconds(350));
        
        timer.stop();
        
        // 应该执行3-4次（考虑时间精度）
        assert(counter >= 3 && counter <= 4);
        
        std::cout << "Periodic task test passed! Executed " << counter << " times" << std::endl;
    }
    
    void testMultipleTasks() {
        std::cout << "Testing multiple tasks..." << std::endl;
        
        // 创建新的Timer实例用于这个测试
        utils::Timer multi_timer;
        
        std::atomic<int> task1_count{0};
        std::atomic<int> task2_count{0};
        std::atomic<int> task3_count{0};
        
        // 添加多个不同类型的任务
        multi_timer.addOnceTask(std::chrono::milliseconds(50), [&task1_count]() {
            task1_count++;
            std::cout << "Task 1 executed" << std::endl;
        });
        
        multi_timer.addOnceTask(std::chrono::milliseconds(100), [&task2_count]() {
            task2_count++;
            std::cout << "Task 2 executed" << std::endl;
        });
        
        multi_timer.addPeriodicTask(
            std::chrono::milliseconds(25),
            std::chrono::milliseconds(75),
            [&task3_count]() {
                task3_count++;
                std::cout << "Task 3 (periodic) executed, count: " << task3_count.load() << std::endl;
            }
        );
        
        multi_timer.start();
        
        // 等待200ms
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        multi_timer.stop();
        
        std::cout << "Task1: " << task1_count << ", Task2: " << task2_count 
                  << ", Task3: " << task3_count << std::endl;
        
        assert(task1_count == 1);  // 一次性任务
        assert(task2_count == 1);  // 一次性任务
        assert(task3_count >= 2);  // 周期性任务，至少执行2次
        
        std::cout << "Multiple tasks test passed!" << std::endl;
    }
    
    void testTaskOrder() {
        std::cout << "Testing task execution order..." << std::endl;
        
        // 创建新的Timer实例用于这个测试
        utils::Timer order_timer;
        
        std::vector<int> execution_order;
        std::mutex order_mutex;
        
        // 添加多个有不同延迟的任务，增加时间间隔
        order_timer.addOnceTask(std::chrono::milliseconds(150), [&execution_order, &order_mutex]() {
            std::lock_guard<std::mutex> lock(order_mutex);
            execution_order.push_back(3);
            std::cout << "Task 3 executed at position " << execution_order.size() << std::endl;
        });
        
        order_timer.addOnceTask(std::chrono::milliseconds(75), [&execution_order, &order_mutex]() {
            std::lock_guard<std::mutex> lock(order_mutex);
            execution_order.push_back(2);
            std::cout << "Task 2 executed at position " << execution_order.size() << std::endl;
        });
        
        order_timer.addOnceTask(std::chrono::milliseconds(25), [&execution_order, &order_mutex]() {
            std::lock_guard<std::mutex> lock(order_mutex);
            execution_order.push_back(1);
            std::cout << "Task 1 executed at position " << execution_order.size() << std::endl;
        });
        
        order_timer.start();
        
        // 等待所有任务完成，增加等待时间
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        order_timer.stop();
        
        // 输出调试信息
        std::cout << "Execution order size: " << execution_order.size() << std::endl;
        for (size_t i = 0; i < execution_order.size(); ++i) {
            std::cout << "Order[" << i << "] = " << execution_order[i] << std::endl;
        }
        
        // 验证执行顺序
        assert(execution_order.size() == 3);
        assert(execution_order[0] == 1);  // 最早执行
        assert(execution_order[1] == 2);  // 第二个执行
        assert(execution_order[2] == 3);  // 最后执行
        
        std::cout << "Task order test passed!" << std::endl;
    }
    
    void testStopAndRestart() {
        std::cout << "Testing stop and restart..." << std::endl;
        
        // 创建新的Timer实例用于这个测试
        utils::Timer restart_timer;
        
        std::atomic<int> counter{0};
        
        restart_timer.addPeriodicTask(
            std::chrono::milliseconds(50),
            std::chrono::milliseconds(50),
            [&counter]() {
                counter++;
            }
        );
        
        restart_timer.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        restart_timer.stop();
        
        int first_count = counter.load();
        assert(first_count >= 1);
        
        // 重新启动
        restart_timer.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        restart_timer.stop();
        
        int second_count = counter.load();
        assert(second_count > first_count);
        
        std::cout << "Stop and restart test passed!" << std::endl;
        std::cout << "First run: " << first_count << ", Second run: " << second_count << std::endl;
    }
    
    void runAllTests() {
        try {
            testOnceTask();
            testPeriodicTask();
            testMultipleTasks();
            testTaskOrder();
            testStopAndRestart();
            
            std::cout << "\n✅ All Timer tests passed!" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
            throw;
        }
    }
};

int main() {
    try {
        TimerTest test;
        test.runAllTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test suite failed: " << e.what() << std::endl;
        return 1;
    }
}
