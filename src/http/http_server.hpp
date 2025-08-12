#pragma once

#include <string>
#include <functional>
#include <vector>
#include <unordered_map>
#include "utils/thread_pool.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "epoller.hpp"
#include "connection.hpp"
#include "router.hpp"

namespace http
{

    class HttpServer
    {
    public:
        explicit HttpServer(int port, size_t thread_count = std::thread::hardware_concurrency());
        ~HttpServer();

        // 服务器主方法
        void run();
        void stop();

        // get方法
        Epoller &getEpoller(); // 获取Epoller实例
        Router &getRouter();   // 获取Router实例
        // 删除连接
        void removeConnection(int fd);

    private:
        // 私有辅助函数
        static void setNoBlocking(int fd);                 // 设置非阻塞
        void addConnection(int fd);                        // 添加新连接
        std::shared_ptr<Connection> getConnection(int fd); // 获取连接的智能指针

        // 成员变量
        int port_;      // 端口
        int server_fd_; // 服务器文件描述符
        bool running_;  // 服务器运行状态

        Epoller epoller_;               // Epoller实例
        utils::ThreadPool thread_pool_; // 线程池实例

        std::unique_ptr<Router> router_;                                   // Router指针
        std::unordered_map<int, std::shared_ptr<Connection>> connections_; // fd到连接的映射
        std::mutex connections_mutex_;                                     // 保护连接映射的互斥锁
    };
}