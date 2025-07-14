#include "websocket_server.hpp"
#include "utils/logger.hpp"
#include <iostream>

WebSocketServer::WebSocketServer() : running_(false) {
    // 设置日志级别
    ws_server_.set_access_channels(websocketpp::log::alevel::all);
    ws_server_.clear_access_channels(websocketpp::log::alevel::frame_payload);
    
    // 初始化ASIO
    ws_server_.init_asio();
    
    // 设置默认的消息处理器
    ws_server_.set_message_handler([this](connection_hdl hdl, message_ptr msg) {
        onMessage(hdl, msg);
    });
    
    ws_server_.set_open_handler([this](connection_hdl hdl) {
        onOpen(hdl);
    });
    
    ws_server_.set_close_handler([this](connection_hdl hdl) {
        onClose(hdl);
    });
}

WebSocketServer::~WebSocketServer() {
    stop();
}

void WebSocketServer::start(int port) {
    try {
        running_ = true;
        
        // 设置监听端口
        ws_server_.listen(port);
        
        // 开始接受连接
        ws_server_.start_accept();
        
        LOG_INFO << "WebSocket server starting on port " << port;
        
        // 在单独的线程中运行服务器
        server_thread_ = std::thread([this]() {
            try {
                ws_server_.run();
            } catch (const std::exception& e) {
                LOG_ERROR << "WebSocket server error: " << e.what();
            }
        });
        
        LOG_INFO << "WebSocket server started successfully";
        
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to start WebSocket server: " << e.what();
        running_ = false;
        throw;
    }
}

void WebSocketServer::stop() {
    if (running_) {
        running_ = false;
        
        LOG_INFO << "Stopping WebSocket server...";
        
        // 停止服务器
        ws_server_.stop();
        
        // 等待服务器线程结束
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        
        LOG_INFO << "WebSocket server stopped";
    }
}

void WebSocketServer::broadcast(const std::string& message) {
    LOG_DEBUG << "Broadcasting message to " << connections_.size() << " clients: " << message;
    
    for (auto& hdl : connections_) {
        try {
            ws_server_.send(hdl, message, websocketpp::frame::opcode::text);
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to send message to client: " << e.what();
        }
    }
}

void WebSocketServer::send(connection_hdl hdl, const std::string& message) {
    try {
        ws_server_.send(hdl, message, websocketpp::frame::opcode::text);
        LOG_DEBUG << "Sent message to client: " << message;
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to send message to specific client: " << e.what();
    }
}

void WebSocketServer::setOnMessage(std::function<void(connection_hdl, message_ptr)> handler) {
    on_message_handler_ = handler;
}

void WebSocketServer::setOnOpen(std::function<void(connection_hdl)> handler) {
    on_open_handler_ = handler;
}

void WebSocketServer::setOnClose(std::function<void(connection_hdl)> handler) {
    on_close_handler_ = handler;
}

void WebSocketServer::onMessage(connection_hdl hdl, message_ptr msg) {
    LOG_DEBUG << "Received message: " << msg->get_payload();
    
    // 调用用户定义的消息处理器
    if (on_message_handler_) {
        on_message_handler_(hdl, msg);
    } else {
        // 默认行为：回显消息
        try {
            ws_server_.send(hdl, "Echo: " + msg->get_payload(), msg->get_opcode());
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to echo message: " << e.what();
        }
    }
}

void WebSocketServer::onOpen(connection_hdl hdl) {
    LOG_INFO << "New WebSocket connection opened";
    connections_.insert(hdl);
    
    // 调用用户定义的连接打开处理器
    if (on_open_handler_) {
        on_open_handler_(hdl);
    }
}

void WebSocketServer::onClose(connection_hdl hdl) {
    LOG_INFO << "WebSocket connection closed";
    connections_.erase(hdl);
    
    // 调用用户定义的连接关闭处理器
    if (on_close_handler_) {
        on_close_handler_(hdl);
    }
}
