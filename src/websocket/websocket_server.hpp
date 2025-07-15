#pragma once
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <thread>
#include <cstdint>
#include <functional>

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
private:
    //初始化服务器，绑定事件处理程序
    void setup_handlers();

    // 事件处理程序
    void on_open(connection_hdl hdl);
    void on_close(connection_hdl hdl);
    void on_message(connection_hdl hdl,websocket_server::message_ptr msg);

    websocket_server server_; // WebSocket服务器实例
    std::thread server_thread_; // 服务器运行线程
};