#include "epoller.hpp"

#include <cerrno>
#include <cstring>
#include <stdexcept>

Epoller::Epoller(int max_events) : epoll_fd_(-1), events_(max_events) {
  epoll_fd_ = epoll_create1(0);  //向内核申请一个内核实例
  if (epoll_fd_ < 0) {
    //如果创建失败，抛出异常，终止构造过程
    throw std::runtime_error("Failed to create epoll instance: " +
                             std::string(strerror(errno)));
  }
}

Epoller::~Epoller() {
  //析构时释放资源
  if (epoll_fd_ >= 0) {
    close(epoll_fd_);
  }
}

bool Epoller::addFd(int fd, uint32_t events) {
  if (fd < 0) {
    return false;
  }
  struct epoll_event event = {0};
  event.data.fd = fd;
  event.events = events;
  //参数：epoll文件描述符，操作类型，要监听的文件描述符，监听事件类型的结构体
  return epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) == 0;
}

bool Epoller::modifyFd(int fd, uint32_t events) {
  if (fd < 0) {
    return false;
  }
  struct epoll_event event = {0};
  event.data.fd = fd;
  event.events = events;
  return epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event) == 0;
}

bool Epoller::removeFd(int fd) {
  if (fd < 0) {
    return false;
  }
  struct epoll_event event = {0};
  return epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &event) == 0;
}

int Epoller::wait(int timeout) {
  //参数：epoll文件描述符、存放事件数组的首地址，数组的大小，超时时间
  return epoll_wait(epoll_fd_, &events_[0], static_cast<int>(events_.size()),
                    timeout);
}

int Epoller::getEventFd(int index) const {
  if (index < 0 || index >= static_cast<int>(events_.size())) {
    throw std::out_of_range("Index out of range in getEventFd");
  }
  return events_[index].data.fd;
}

uint32_t Epoller::getEvents(int index) const {
  if (index < 0 || index >= static_cast<int>(events_.size())) {
    throw std::out_of_range("Index out of range in getEvents");
  }
  return events_[index].events;
}