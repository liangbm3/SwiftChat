#pragma once

#include <chrono>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>

namespace utils
{

    class Timer
    {
    public:
        struct Task
        {
            std::chrono::steady_clock::time_point execution_time; // 任务执行时间
            std::function<void()> func;                           // 任务函数，回调函数
            bool is_periodic;                                     // 是否为周期性任务
            std::chrono::milliseconds period;                     // 周期时间
            // 执行时间早的任务有更高优先级，小顶堆，使用 >
            bool operator>(const Task &other) const
            {
                return execution_time > other.execution_time; // 执行时间晚的任务 > 执行时间早的任务
            }
        };
        explicit Timer();
        ~Timer();

        // 添加一次性定时任务
        void addOnceTask(std::chrono::milliseconds delay, std::function<void()> func);

        // 添加周期性定时任务
        void addPeriodicTask(std::chrono::milliseconds delay,
                             std::chrono::milliseconds period,
                             std::function<void()> func);

        // 启动定时器线程
        void start();

        // 停止定时器线程
        void stop();

    private:
        void processTimerTasks();
        void scheduleNextTask();

        std::priority_queue<Task, std::vector<Task>, std::greater<>> task_queue_; // 统一的任务优先队列
        std::mutex mutex_;                 // 互斥锁，保护任务队列
        std::condition_variable cond_var_; // 条件变量，用于通知定时器线程
        std::thread timer_thread_;         // 定时器线程
        bool running_;                     // 定时器是否在运行
    };

}