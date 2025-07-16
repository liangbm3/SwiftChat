#pragma once
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <thread>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <set>
#include <string>
#include <string>
#include <set>

using websocket_server = websocketpp::server<websocketpp::config::asio>;
using connection_hdl = websocketpp::connection_hdl;

// 为 connection_hdl 定义哈希函数和相等比较函数
struct ConnectionHdlHash {
    std::size_t operator()(const connection_hdl& hdl) const {
        return std::hash<void*>()(hdl.lock().get());
    }
};

struct ConnectionHdlEqual {
    bool operator()(const connection_hdl& a, const connection_hdl& b) const {
        return a.lock() == b.lock();
    }
};

using websocket_server = websocketpp::server<websocketpp::config::asio>;
using connection_hdl = websocketpp::connection_hdl;

class WebSocketServer
{
public:
    explicit WebSocketServer();
    ~WebSocketServer();

    // 在指定端口启动WebSocket服务器
    void run(uint16_t port);

    // 停止WebSocket服务器
    void stop();

    //向指定用户发送信息
    void send_to_user(const std::string &user_id, const std::string &message);

    // 广播消息到房间
    void broadcast_to_room(const std::string &room_id, const std::string &message);
private:
    //初始化服务器，绑定事件处理程序
    void setup_handlers();

    // 事件处理程序
    void on_open(connection_hdl hdl);
    void on_close(connection_hdl hdl);
    void on_message(connection_hdl hdl,websocket_server::message_ptr msg);

    websocket_server server_; // WebSocket服务器实例
    std::thread server_thread_; // 服务器运行线程
    std::mutex connection_mutex_; // 保护连接的互斥锁
    std::unordered_map<std::string, connection_hdl> user_connections_; // 用户ID到连接句柄的映射
    std::unordered_map<connection_hdl, std::string, ConnectionHdlHash, ConnectionHdlEqual> connection_users_; // 连接句柄到用户ID的映射
};