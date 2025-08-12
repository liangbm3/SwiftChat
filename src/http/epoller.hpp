#pragma once

#include <sys/epoll.h>
#include <vector>
#include <unistd.h>

class Epoller
{
public:
    explicit Epoller(int max_events = 1024);
    ~Epoller();

    //禁止拷贝和赋值，确保资源管理的唯一性
    Epoller(const Epoller &) = delete;
    Epoller &operator=(const Epoller &) = delete;

    //文件描述符管理
    bool addFd(int fd, uint32_t events);// 添加文件描述符
    bool modifyFd(int fd, uint32_t events);// 修改文件描述符
    bool removeFd(int fd);// 移除文件描述符

    // 等待事件
    int wait(int timeout = -1);

    //获取事件结果
    int getEventFd(int index) const;// 获取事件文件描述符
    uint32_t getEvents(int index) const;// 获取事件类型


private:
    int epoll_fd_;//epoll实例的文件描述符
    std::vector<struct epoll_event> events_;//存储触发事件的缓冲区
};