#pragma once

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <string>
#include <set>
#include <thread>
#include <functional>

class WebSocketServer {
public:
    typedef websocketpp::server<websocketpp::config::asio> server;
    typedef server::message_ptr message_ptr;
    typedef websocketpp::connection_hdl connection_hdl;

    WebSocketServer();
    ~WebSocketServer();

    void start(int port);
    void stop();
    void broadcast(const std::string& message);
    void send(connection_hdl hdl, const std::string& message);
    
    // 设置消息处理回调
    void setOnMessage(std::function<void(connection_hdl, message_ptr)> handler);
    void setOnOpen(std::function<void(connection_hdl)> handler);
    void setOnClose(std::function<void(connection_hdl)> handler);

private:
    server ws_server_;
    std::set<connection_hdl, std::owner_less<connection_hdl>> connections_;
    std::thread server_thread_;
    bool running_;

    void onMessage(connection_hdl hdl, message_ptr msg);
    void onOpen(connection_hdl hdl);
    void onClose(connection_hdl hdl);

    // 用户定义的回调函数
    std::function<void(connection_hdl, message_ptr)> on_message_handler_;
    std::function<void(connection_hdl)> on_open_handler_;
    std::function<void(connection_hdl)> on_close_handler_;
};
