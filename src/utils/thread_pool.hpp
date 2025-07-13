#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>

namespace utils
{

class ThreadPool
{
private:
    std::vector<std::thread> workers;        // 工作线程
    std::queue<std::function<void()>> tasks; // 任务队列
    std::mutex queue_mutex;                  // 队列锁
    std::condition_variable condition;
    bool stop;

public:
    ThreadPool(size_t num_threads) : stop(false)
    {
        for (size_t i = 0; i < num_threads; i++) // 创建并启动相应数量的线程
        {
            workers.emplace_back([this]
                                 {
            while(true)
            {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);//对任务队列加锁
                    //传入锁和一个谓词，谓词是一个返回布尔值的lambda，如果条件满足stop为true或者
                    //任务队列不为空才能继续进行，否则wait会解锁mutex并让当前线程休眠
                    //其他线程调用notify_one()或者notify_all()这个休眠的线程才会唤醒
                    condition.wait(lock,[this]{return stop || !tasks.empty();});
                    //如果线程池停止了，而且队列为空就不用继续执行了
                    if(stop&&tasks.empty())
                    {
                        return;
                    }
                    //从队头取出一个任务执行
                    task=std::move(tasks.front());
                    //弹出任务
                    tasks.pop();
                }
                task();
            } });
        }
    }
    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true; // 设置停止标志位
        }
        condition.notify_all(); // 唤醒所有线程
        for (std::thread &worker : workers)
        {
            worker.join(); // 主线程在这里阻塞等待线程执行完毕
        }
    }
    // 可以接受任意函数F和其对应的参数Args...
    template <class F, class... Args>
    // invoke_result用来推导函数f被调用后的返回值类型
    // future用来获取异步执行的结果
    auto enqueue(F &&f, Args &&...args) -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        using return_type = typename std::invoke_result<F, Args...>::type;
        // packaged_task可以将一个可调用对象包装起来，使其可以被异步调用，
        // 当执行这个packaged_task时，返回值会自动存入一个与之关联的future对象中
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            // bind将函数f和它的参数绑定在一起，生成一个无参数的可调用对象
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        // 从package_task中获取一个future对象，调用enqueue的线程会得到这个future，并可以在未来某个时刻
        // 通过它来等待任务完成并获取返回值
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop)
            {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            // tasks中添加的不是packaged_task本身，而是一个新的lambda表达式，这个lambda表达式捕获了
            // shared_ptr，工作线程执行这个表达式会调用(*task)();来执行原始任务
            tasks.emplace([task]()
                          { (*task)(); });
        }
        condition.notify_one();
        return res; // 返回future给调用者
    }
};

} // namespace utils