#pragma once

#include <sys/epoll.h>
#include <vector>
#include <unistd.h>

class Epoller
{
public:
    explicit Epoller(int max_events = 1024);
    ~Epoller();
    //禁止拷贝和赋值
    Epoller(const Epoller &) = delete;
    Epoller &operator=(const Epoller &) = delete;

    bool addFd(int fd, uint32_t events);
    bool modifyFd(int fd, uint32_t events);
    bool removeFd(int fd);
    int wait(int timeout = -1);
    int getEventFd(int index) const;
    uint32_t getEvents(int index) const;


private:
    int epoll_fd_;
    std::vector<struct epoll_event> events_;
};