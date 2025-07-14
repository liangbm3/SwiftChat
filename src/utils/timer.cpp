#include "timer.hpp"
#include <iostream>

namespace utils
{
    Timer::Timer() : running_(false)
    {
    }

    Timer::~Timer()
    {
        stop();
    }

    void Timer::addOnceTask(std::chrono::milliseconds delay, std::function<void()> func)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        Task task;
        task.execution_time = std::chrono::steady_clock::now() + delay;
        task.func = std::move(func);
        task.is_periodic = false;
        task_queue_.push(task);
        cond_var_.notify_one(); // 通知定时器线程有新任务
    }

    void Timer::addPeriodicTask(std::chrono::milliseconds delay,
                                std::chrono::milliseconds period,
                                std::function<void()> func)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        Task task;
        task.execution_time = std::chrono::steady_clock::now() + delay;
        task.func = std::move(func);
        task.is_periodic = true;
        task.period = period;
        task_queue_.push(task);
        cond_var_.notify_one(); // 通知定时器线程有新任务
    }

    void Timer::start()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_)
        {
            running_ = true;
            timer_thread_ = std::thread([this]
                                        { processTimerTasks(); });
        }
    }

    void Timer::stop()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            running_ = false;
            cond_var_.notify_all(); // 唤醒等待的线程
        }

        if (timer_thread_.joinable())
        {
            timer_thread_.join(); // 等待线程结束
        }
    }

    void Timer::processTimerTasks()
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock(mutex_);

            if (!running_)
                break;

            // 任务队列为空时等待
            if (task_queue_.empty())
            {
                // wait的第二个参数是一个谓词，只有当谓词返回true时才会继续执行，防止虚假唤醒
                cond_var_.wait(lock, [this]
                               { return !running_ || !task_queue_.empty(); });
                continue;
            }

            // 获取最早的任务
            auto now = std::chrono::steady_clock::now();
            Task next_task = task_queue_.top();

            // 如果任务还没到执行时间，等待到指定时间
            if (next_task.execution_time > now)
            {
                auto wait_result = cond_var_.wait_until(lock, next_task.execution_time);
                // 检查是否因为停止而被唤醒
                if (!running_)
                {
                    break;
                }
                // 重新获取当前时间并检查队列状态
                now = std::chrono::steady_clock::now();
                if (task_queue_.empty())
                {
                    continue;
                }
                // 重新获取队列顶部任务（可能已经改变）
                next_task = task_queue_.top();
                // 如果还是没到时间，继续等待
                if (next_task.execution_time > now)
                {
                    continue;
                }
            }

            // 移除即将执行的任务
            task_queue_.pop();

            // 如果是周期性任务，重新调度
            if (next_task.is_periodic)
            {
                Task periodic_task = next_task;
                periodic_task.execution_time = now + next_task.period;
                task_queue_.push(periodic_task);
            }

            // 解锁并执行任务
            lock.unlock();

            try
            {
                next_task.func();
            }
            catch (const std::exception &e)
            {
                // 捕获任务执行中的异常
                std::cerr << "Timer task exception: " << e.what() << std::endl;
            }
        }
    }
}