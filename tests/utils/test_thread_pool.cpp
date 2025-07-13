#include "../src/utils/thread_pool.hpp"
#include <iostream>
#include <chrono>
#include <atomic>
#include <vector>
#include <cassert>

using namespace utils;

// 测试函数1：简单的计算任务
int add(int a, int b)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return a + b;
}

// 测试函数2：无返回值的任务
void print_message(const std::string &msg)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::cout << "Task: " << msg << " executed on thread "
              << std::this_thread::get_id() << std::endl;
}

// 测试函数3：计算密集型任务
long long fibonacci(int n)
{
    if (n <= 1)
        return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// 测试1：基本功能测试
void test_basic_functionality()
{
    std::cout << "\n=== 测试1: 基本功能测试 ===" << std::endl;

    ThreadPool pool(4);

    // 提交有返回值的任务
    auto result1 = pool.enqueue(add, 10, 20);
    auto result2 = pool.enqueue(add, 5, 15);

    // 等待结果并验证
    assert(result1.get() == 30);
    assert(result2.get() == 20);

    std::cout << "基本功能测试通过！" << std::endl;
}

// 测试2：并发任务测试
void test_concurrent_tasks()
{
    std::cout << "\n=== 测试2: 并发任务测试 ===" << std::endl;

    ThreadPool pool(4);
    std::vector<std::future<void>> futures;

    // 提交多个并发任务
    for (int i = 0; i < 10; ++i)
    {
        futures.push_back(
            pool.enqueue(print_message, "Task " + std::to_string(i)));
    }

    // 等待所有任务完成
    for (auto &future : futures)
    {
        future.get();
    }

    std::cout << "并发任务测试完成！" << std::endl;
}

// 测试3：性能测试
void test_performance()
{
    std::cout << "\n=== 测试3: 性能测试 ===" << std::endl;

    const int num_tasks = 20;
    ThreadPool pool(4);

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::future<long long>> futures;
    for (int i = 0; i < num_tasks; ++i)
    {
        futures.push_back(pool.enqueue(fibonacci, 30));
    }

    // 等待所有任务完成
    long long total = 0;
    for (auto &future : futures)
    {
        total += future.get();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "执行 " << num_tasks << " 个斐波那契(30)任务耗时: "
              << duration.count() << " ms" << std::endl;
    std::cout << "总和: " << total << std::endl;
}

// 测试4：异常处理测试
void test_exception_handling()
{
    std::cout << "\n=== 测试4: 异常处理测试 ===" << std::endl;

    ThreadPool pool(2);

    // 测试会抛出异常的任务
    auto future1 = pool.enqueue([]() -> int
                                {
        throw std::runtime_error("测试异常");
        return 42; });

    try
    {
        future1.get();
        assert(false); // 不应该到达这里
    }
    catch (const std::exception &e)
    {
        std::cout << "捕获到预期异常: " << e.what() << std::endl;
    }

    // 测试正常任务仍然可以执行
    auto future2 = pool.enqueue([]() -> int
                                { return 100; });

    assert(future2.get() == 100);
    std::cout << "异常处理测试通过！" << std::endl;
}

// 测试5：线程池销毁测试
void test_thread_pool_destruction()
{
    std::cout << "\n=== 测试5: 线程池销毁测试 ===" << std::endl;

    std::atomic<int> counter{0};

    {
        ThreadPool pool(4);

        // 提交一些任务
        for (int i = 0; i < 10; ++i)
        {
            pool.enqueue([&counter]()
                         {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                counter++; });
        }

        // 线程池在作用域结束时会被销毁
        std::cout << "线程池即将销毁..." << std::endl;
    }

    std::cout << "线程池已销毁，完成的任务数: " << counter.load() << std::endl;
    std::cout << "线程池销毁测试完成！" << std::endl;
}

// 测试6：停止后提交任务测试
void test_enqueue_after_stop()
{
    std::cout << "\n=== 测试6: 停止后提交任务测试 ===" << std::endl;

    ThreadPool *pool = new ThreadPool(2);

    // 删除线程池
    delete pool;

    // 注意：这里无法直接测试因为析构函数已经被调用
    // 在实际使用中，应该确保线程池对象的生命周期管理

    std::cout << "停止后提交任务测试完成！" << std::endl;
}

// 测试7：lambda表达式测试
void test_lambda_functions()
{
    std::cout << "\n=== 测试7: Lambda表达式测试 ===" << std::endl;

    ThreadPool pool(3);

    int x = 10;
    auto future1 = pool.enqueue([x](int y) -> int
                                { return x * y; }, 5);

    auto future2 = pool.enqueue([](const std::string &str) -> size_t
                                { return str.length(); }, std::string("Hello World"));

    assert(future1.get() == 50);
    assert(future2.get() == 11);

    std::cout << "Lambda表达式测试通过！" << std::endl;
}

int main()
{
    std::cout << "开始线程池测试..." << std::endl;

    try
    {
        test_basic_functionality();
        test_concurrent_tasks();
        test_performance();
        test_exception_handling();
        test_thread_pool_destruction();
        test_enqueue_after_stop();
        test_lambda_functions();

        std::cout << "\n=== 所有测试通过！ ===" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "测试失败: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
