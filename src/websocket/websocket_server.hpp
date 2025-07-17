#pragma once
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <nlohmann/json.hpp>
#include <thread>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <set>
#include <string>

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
    void broadcast_to_room(const std::string &room_id, const std::string &message, const std::string &exclude_user_id);
private:
    //初始化服务器，绑定事件处理程序
    void setup_handlers();

    // 事件处理程序
    void on_open(connection_hdl hdl);
    void on_close(connection_hdl hdl);
    void on_message(connection_hdl hdl,websocket_server::message_ptr msg);
    
    // 消息处理辅助方法
    void handle_authenticated_message(connection_hdl hdl, const std::string& user_id, const nlohmann::json& message);
    void handle_join_room(connection_hdl hdl, const std::string& user_id, const nlohmann::json& message);
    void handle_leave_room(connection_hdl hdl, const std::string& user_id, const nlohmann::json& message);
    void handle_chat_message(connection_hdl hdl, const std::string& user_id, const nlohmann::json& message);
    
    // 房间管理辅助方法
    void join_room(const std::string& user_id, const std::string& room_id);
    void leave_room(const std::string& user_id, const std::string& room_id);
    void send_error(connection_hdl hdl, const std::string& error_message);

    websocket_server server_; // WebSocket服务器实例
    std::thread server_thread_; // 服务器运行线程
    std::mutex connection_mutex_; // 保护连接的互斥锁
    
    // 用户和连接管理
    std::unordered_map<std::string, connection_hdl> user_connections_; // 用户ID到连接句柄的映射
    std::unordered_map<connection_hdl, std::string, ConnectionHdlHash, ConnectionHdlEqual> connection_users_; // 连接句柄到用户ID的映射
    
    // 房间管理
    std::unordered_map<std::string, std::set<std::string>> room_members_; // 房间ID到用户ID集合的映射
    std::unordered_map<std::string, std::string> user_current_room_; // 用户ID到当前房间ID的映射
};