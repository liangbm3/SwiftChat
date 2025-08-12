#pragma once

#include <memory>
#include <string>
#include <mutex>
#include <http/http_request.hpp>

namespace http
{
    class HttpServer;
    class Router;
}

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    // 连接状态
    enum class State
    {
        HTTP,
        WEBSOCKET,
        CLOSING
    };

    // 构造函数，通过依赖注入获取他需要协作的组件
    Connection(int fd, http::HttpServer *server, http::Router *router);
    ~Connection();

    // 主入口函数
    void handleEvent();

    // 获取文件描述符
    int getFd() const;

private:
    // 处理HTTP协议数据的私有方法
    void processHttpData();
    // 处理WebSocket协议数据的私有方法
    void processWebSocketData();
    // 关闭连接的私有方法
    void closeConnection();
    // 发送响应的辅助方法
    bool sendResponse(const std::string &response);
    // WebSocket 相关方法
    bool isWebSocketUpgradeRequest(const http::HttpRequest &request);
    bool handleWebSocketHandshake(int client_fd, const http::HttpRequest &request);
    std::string generateWebSocketAcceptKey(const std::string &websocket_key);

    // 成员变量
    int fd_;
    http::HttpServer *server_;
    http::Router *router_;

    State state_;              // 连接状态
    std::string read_buffer_;  // 读取缓冲区
    std::string write_buffer_; // 写入缓冲区

    std::mutex mutex_; // 保护内部状态的互斥锁
};