#include "http_server.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "utils/logger.hpp"

namespace http {

HttpServer::HttpServer(int port, size_t thread_count)
    : port_(port),
      running_(false),
      thread_pool_(thread_count),
      epoller_(),  //默认初始化
      router_(std::make_unique<Router>()) {
  // 忽略SIGPIPE信号，避免写入已关闭的套接字导致程序终止
  signal(SIGPIPE, SIG_IGN);

  // 创建套接字
  server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd_ < 0) {
    LOG_ERROR << "Failed to create socket: " << strerror(errno);
    throw std::runtime_error("Failed to create socket");
  }

  // 设置套接字选项
  int opt = 1;
  // 允许服务器在关闭后立即重启，即使之前的连接还处于TIME_WAIT状态，否则会绑定失败
  if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    LOG_ERROR << "Failed to set socket options: " << strerror(errno);
    close(server_fd_);
    throw std::runtime_error("Failed to set socket options");
  }

  // 性能优化：设置套接字缓冲区大小
  int send_buffer = 65536;  // 64KB发送缓冲区
  int recv_buffer = 65536;  // 64KB接收缓冲区
  if (setsockopt(server_fd_, SOL_SOCKET, SO_SNDBUF, &send_buffer,
                 sizeof(send_buffer)) < 0) {
    LOG_WARN << "Failed to set send buffer size: " << strerror(errno);
  }
  if (setsockopt(server_fd_, SOL_SOCKET, SO_RCVBUF, &recv_buffer,
                 sizeof(recv_buffer)) < 0) {
    LOG_WARN << "Failed to set receive buffer size: " << strerror(errno);
  }

  // 启用TCP_NODELAY，禁用Nagle算法以减少延迟
  if (setsockopt(server_fd_, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
    LOG_WARN << "Failed to set TCP_NODELAY: " << strerror(errno);
  }

  // 绑定套接字到指定端口
  struct sockaddr_in server_addr;
  std::memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;          // IPv4
  server_addr.sin_addr.s_addr = INADDR_ANY;  // 绑定到所有可用地址
  server_addr.sin_port = htons(port_);  // 转换端口号为网络字节序
  if (bind(server_fd_, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    LOG_ERROR << "Failed to bind socket: " << strerror(errno);
    close(server_fd_);
    throw std::runtime_error("Failed to bind socket");
  }

  // 开始监听连接
  if (listen(server_fd_, SOMAXCONN) < 0) {
    LOG_ERROR << "Failed to listen on socket: " << strerror(errno);
    close(server_fd_);
    throw std::runtime_error("Failed to listen on socket");
  }

  setNoBlocking(server_fd_);  // 设置非阻塞模式
  // 将监听套接字添加到epoll中，监听读事件，使用ET
  if (!epoller_.addFd(server_fd_, EPOLLIN | EPOLLET)) {
    LOG_ERROR << "Failed to add server socket to epoll: " << strerror(errno);
    close(server_fd_);
    throw std::runtime_error("Failed to add server socket to epoll");
  }
}

HttpServer::~HttpServer() {
  stop();
  if (server_fd_ >= 0) close(server_fd_);
}

void HttpServer::run() {
  running_ = true;
  LOG_INFO << "HTTP server is running on port " << port_;

  while (running_) {
    // 等待epoll事件（设置1秒超时，以便能够响应关闭信号）
    int event_count = epoller_.wait(1000);  // 1000ms超时
    if (event_count < 0) {
      if (errno == EINTR) {
        continue;  // 被信号中断，继续等待
      }
      LOG_ERROR << "Epoll wait error: " << strerror(errno);
      break;  // 其他错误，退出循环
    } else if (event_count == 0) {
      // 超时，没有事件，继续循环（这会检查running_标志）
      continue;
    }
    // 遍历所有就绪事件
    for (int i = 0; i < event_count; i++) {
      int fd = epoller_.getEventFd(i);
      uint32_t events = epoller_.getEvents(i);
      if (fd == server_fd_)  // 新连接到达
      {
        // ET模式需要循环accept直到没有连接
        while (true) {
          sockaddr_in client_addr{};
          socklen_t client_addr_len = sizeof(client_addr);
          int client_fd = accept(server_fd_, (struct sockaddr *)&client_addr,
                                 &client_addr_len);
          if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
              break;  // 没有更多连接，退出循环
            }
            LOG_ERROR << "Failed to accept connection: " << strerror(errno);
            break;
          }

          // 减少日志输出，避免DNS查找
          LOG_DEBUG << "Accepted new connection from "
                    << ((client_addr.sin_addr.s_addr >> 0) & 0xFF) << "."
                    << ((client_addr.sin_addr.s_addr >> 8) & 0xFF) << "."
                    << ((client_addr.sin_addr.s_addr >> 16) & 0xFF) << "."
                    << ((client_addr.sin_addr.s_addr >> 24) & 0xFF) << ":"
                    << ntohs(client_addr.sin_port);

          // 添加新连接（这会设置非阻塞、TCP_NODELAY和添加到epoll）
          addConnection(client_fd);
        }
      } else  // 已有连接的事件
      {
        // 错误或连接关闭
        if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
          removeConnection(fd);       //移除连接
        } else if (events & EPOLLIN)  //有数据可读
        {
          auto conn = getConnection(fd);
          if (conn) {
            // 从epoll中暂时移除，防止在处理时被其他线程重复触发
            epoller_.removeFd(fd);
            thread_pool_.enqueue([conn]() {
              conn->handleEvent();  // 处理连接事件
            });
          } else {
            LOG_WARN << "Failed to get connection for fd " << fd;
            epoller_.removeFd(fd);  // 移除无效连接
            close(fd);              // 关闭套接字
          }
        }
      }
    }
  }
  LOG_INFO << "HTTP server main loop exited";
}

void HttpServer::stop() {
  running_ = false;
  if (server_fd_ >= 0) {
    // 关闭服务器套接字以中断accept()调用
    if (shutdown(server_fd_, SHUT_RDWR) < 0) {
      LOG_WARN << "Failed to shutdown server socket: " << strerror(errno);
    }
    close(server_fd_);
    server_fd_ = -1;
  }
}

Router &HttpServer::getRouter() { return *router_; }

Epoller &HttpServer::getEpoller() { return epoller_; }

void HttpServer::addConnection(int fd) {
  // 把套接字fd设置为非阻塞模式，后续对fd的读写不会阻塞线程
  setNoBlocking(fd);
  int opt = 1;
  // 关闭Nagle算法，启用TCP_NODELAY，让小包立即发送
  if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
    LOG_WARN << "Failed to set TCP_NODELAY for fd " << fd << ": "
             << strerror(errno);
  }
  std::shared_ptr<Connection> conn;
  {  // 进入互斥区
    std::lock_guard<std::mutex> lock(connections_mutex_);
    conn = std::make_shared<Connection>(fd, this, router_.get());
    connections_[fd] = conn;
  }
  LOG_INFO << "New connection added for fd: " << fd;
  if (!epoller_.addFd(fd, EPOLLIN | EPOLLET | EPOLLRDHUP)) {
    LOG_ERROR << "Failed to add fd " << fd << " to epoll";
    // 如果添加到epoll失败，需要从连接映射中移除
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.erase(fd);
  }
}

void HttpServer::removeConnection(int fd) {
  std::lock_guard<std::mutex> lock(connections_mutex_);
  if (connections_.count(fd)) {
    LOG_INFO << "Connection removed for fd: " << fd;
    epoller_.removeFd(fd);
    connections_.erase(fd);
    // Connection对象会在shared_ptr引用计数变为0时自动析构，析构函数会close(fd);
  }
}

std::shared_ptr<Connection> HttpServer::getConnection(int fd) {
  std::lock_guard<std::mutex> lock(connections_mutex_);
  if (connections_.count(fd)) {
    return connections_[fd];
  }
  return nullptr;
}

void HttpServer::setNoBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    LOG_ERROR << "Failed to get file descriptor flags: " << strerror(errno);
    return;
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    LOG_ERROR << "Failed to set file descriptor to non-blocking: "
              << strerror(errno);
  }
}
}  // namespace http
