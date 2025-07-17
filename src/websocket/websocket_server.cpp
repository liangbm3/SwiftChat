#include "websocket_server.hpp"
#include "utils/logger.hpp"
#include <jwt-cpp/jwt.h>
#include <nlohmann/json.hpp>
#include <ctime>

using json = nlohmann::json;

WebSocketServer::WebSocketServer()
{
    // 关闭websocketpp的日志
    server_.clear_access_channels(websocketpp::log::alevel::all);
    server_.clear_error_channels(websocketpp::log::elevel::all);

    // 初始化Asio
    server_.init_asio();

    // 设置重用地址选项
    server_.set_reuse_addr(true);

    // 绑定事件处理程序
    setup_handlers();
}

WebSocketServer::~WebSocketServer()
{
    // 停止服务器线程
    if (server_thread_.joinable())
    {
        stop();
    }
}

void WebSocketServer::setup_handlers()
{
    using websocketpp::lib::bind;
    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::placeholders::_2;

    // 新连接建立时调用on_open
    server_.set_open_handler(bind(&WebSocketServer::on_open, this, _1));
    // 连接关闭时调用on_close
    server_.set_close_handler(bind(&WebSocketServer::on_close, this, _1));
    // 接收到消息时调用on_message
    server_.set_message_handler(bind(&WebSocketServer::on_message, this, _1, _2));
}

void WebSocketServer::run(uint16_t port)
{
    // 在一个新线程中启动服务器，避免阻塞主线程
    server_thread_ = std::thread([this, port]()
                                 {
        try
        {
            LOG_INFO << "Starting WebSocket server on port " << port;
            
            // 设置监听端口
            websocketpp::lib::error_code ec;
            server_.listen(port, ec);
            if (ec) {
                LOG_ERROR << "WebSocket server listen error: " << ec.message();
                return;
            }
            
            LOG_INFO << "WebSocket server listening on port " << port;

            // 开始接受连接
            server_.start_accept(ec);
            if (ec) {
                LOG_ERROR << "WebSocket server start_accept error: " << ec.message();
                return;
            }

            // 运行Asio事件循环
            server_.run();
            LOG_INFO << "WebSocket server event loop exited";
        }
        catch (const websocketpp::exception &e)
        {
            LOG_ERROR << "WebSocket server error: " << e.what();
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "WebSocket server error: " << e.what();
        } });
}

void WebSocketServer::stop()
{
    LOG_INFO << "Stopping WebSocket server...";
    
    try {
        // 停止监听新连接
        if (server_.is_listening()) {
            websocketpp::lib::error_code ec;
            server_.stop_listening(ec);
            if (ec) {
                LOG_ERROR << "Error stopping listening: " << ec.message();
            }
        }
        
        // 关闭所有现有连接
        {
            std::lock_guard<std::mutex> lock(connection_mutex_);
            for (const auto& pair : connection_users_) {
                websocketpp::lib::error_code ec;
                server_.close(pair.first, websocketpp::close::status::going_away, "Server shutdown", ec);
                if (ec) {
                    LOG_ERROR << "Error closing connection: " << ec.message();
                }
            }
        }
        
        // 停止IO事件循环
        server_.stop();
        
        // 等待服务器线程结束
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        
        LOG_INFO << "WebSocket server stopped successfully";
    }
    catch (const std::exception& e) {
        LOG_ERROR << "Error stopping WebSocket server: " << e.what();
    }
}

//-----事件处理函数的具体实现-----

void WebSocketServer::on_open(connection_hdl hdl)
{
    LOG_INFO << "New WebSocket connection opened";
    // 可以在这里处理新连接的初始化逻辑
}

void WebSocketServer::on_close(connection_hdl hdl)
{
    // 使用互斥锁保护共享数据
    std::lock_guard<std::mutex> lock(connection_mutex_);
    // 检查这个连接是否已经认证过了
    auto it = connection_users_.find(hdl);
    if (it != connection_users_.end())
    {
        std::string user_id = it->second;
        LOG_INFO << "WebSocket connection closed for user: " << user_id;
        
        // 从用户当前房间中移除
        auto room_it = user_current_room_.find(user_id);
        if (room_it != user_current_room_.end())
        {
            leave_room(user_id, room_it->second);
        }
        
        // 从用户连接映射中移除
        user_connections_.erase(user_id);
        connection_users_.erase(it);
    }
    else
    {
        LOG_INFO << "WebSocket connection closed for unknown user";
    }
}

void WebSocketServer::on_message(connection_hdl hdl, websocket_server::message_ptr msg)
{
    // 使用互斥锁保护共享数据
    std::lock_guard<std::mutex> lock(connection_mutex_);
    // 检查连接是否已经认证
    auto it = connection_users_.find(hdl);
    if (it == connection_users_.end())
    {
        // 处理未认证的连接，期望收到的认证消息
        try
        {
            auto json_msg = json::parse(msg->get_payload());
            if (json_msg.value("type", "") == "auth")
            {
                // 处理认证消息
                std::string user_id = json_msg.at("user_id").get<std::string>();
                std::string token = json_msg.at("token").get<std::string>();

                // 验证JWT令牌
                const char *secret_key_cstr = std::getenv("JWT_SECRET");
                if (!secret_key_cstr)
                {
                    LOG_ERROR << "FATAL: JWT_SECRET not set for verification.";
                    json error_response = {
                        {"type", "auth_error"},
                        {"message", "Server configuration error"}
                    };
                    server_.send(hdl, error_response.dump(), websocketpp::frame::opcode::text);
                    server_.close(hdl, websocketpp::close::status::internal_endpoint_error, "Server configuration error");
                    return;
                }
                std::string secret_key(secret_key_cstr);

                jwt::decoded_jwt decoded_token = jwt::decode(token);
                auto verifier = jwt::verify()
                                    .allow_algorithm(jwt::algorithm::hs256{secret_key})
                                    .with_issuer("SwiftChat");

                verifier.verify(decoded_token);

                // 认证通过，保存连接和用户ID映射
                user_connections_[user_id] = hdl;
                connection_users_[hdl] = user_id;

                LOG_INFO << "WebSocket connection authenticated for user: " << user_id;
                json response = {
                    {"type", "auth_success"},
                    {"message", "Authentication successful"},
                    {"user_id", user_id}
                };
                server_.send(hdl, response.dump(), websocketpp::frame::opcode::text);
            }
            else
            {
                // 第一个消息不是auth类型则断开连接
                LOG_ERROR << "First message must be an authentication message.";
                server_.close(hdl, websocketpp::close::status::policy_violation, "First message must be an authentication message.");
            }
        }
        catch (const json::exception &e)
        {
            LOG_ERROR << "JSON parsing error: " << e.what();
            server_.close(hdl, websocketpp::close::status::invalid_payload, "Invalid JSON format");
        }
        catch (const jwt::error::token_verification_exception &e)
        {
            LOG_ERROR << "JWT verification failed: " << e.what();
            json error_response = {
                {"type", "auth_error"},
                {"message", "Invalid token"}
            };
            server_.send(hdl, error_response.dump(), websocketpp::frame::opcode::text);
            server_.close(hdl, websocketpp::close::status::policy_violation, "Invalid token");
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "WebSocket message handling error: " << e.what();
            json error_response = {
                {"type", "error"},
                {"message", "Internal server error"}
            };
            server_.send(hdl, error_response.dump(), websocketpp::frame::opcode::text);
            server_.close(hdl, websocketpp::close::status::internal_endpoint_error, "Internal server error");
        }
    }
    else
    {
        // 处理已认证用户的消息
        std::string user_id = it->second;
        try
        {
            auto json_msg = json::parse(msg->get_payload());
            handle_authenticated_message(hdl, user_id, json_msg);
        }
        catch (const json::exception &e)
        {
            LOG_ERROR << "JSON parsing error from user " << user_id << ": " << e.what();
            send_error(hdl, "Invalid JSON format");
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "Message handling error from user " << user_id << ": " << e.what();
            send_error(hdl, "Message processing error");
        }
    }
}

void WebSocketServer::handle_authenticated_message(connection_hdl hdl, const std::string& user_id, const json& message)
{
    std::string msg_type = message.value("type", "");
    
    LOG_INFO << "Received message type '" << msg_type << "' from user: " << user_id;
    
    if (msg_type == "join_room")
    {
        handle_join_room(hdl, user_id, message);
    }
    else if (msg_type == "leave_room")
    {
        handle_leave_room(hdl, user_id, message);
    }
    else if (msg_type == "chat_message")
    {
        handle_chat_message(hdl, user_id, message);
    }
    else if (msg_type == "ping")
    {
        // 处理心跳消息
        json pong_response = {
            {"type", "pong"},
            {"timestamp", std::time(nullptr)}
        };
        server_.send(hdl, pong_response.dump(), websocketpp::frame::opcode::text);
    }
    else
    {
        LOG_WARN << "Unknown message type '" << msg_type << "' from user: " << user_id;
        send_error(hdl, "Unknown message type: " + msg_type);
    }
}

void WebSocketServer::handle_join_room(connection_hdl hdl, const std::string& user_id, const json& message)
{
    try
    {
        std::string room_id = message.at("room_id").get<std::string>();
        
        // 如果用户已经在其他房间，先离开
        auto current_room_it = user_current_room_.find(user_id);
        if (current_room_it != user_current_room_.end())
        {
            leave_room(user_id, current_room_it->second);
        }
        
        // 加入新房间
        join_room(user_id, room_id);
        
        LOG_INFO << "User " << user_id << " joined room: " << room_id;
        
        // 发送成功响应给用户
        json response = {
            {"type", "room_joined"},
            {"room_id", room_id},
            {"message", "Successfully joined room"}
        };
        server_.send(hdl, response.dump(), websocketpp::frame::opcode::text);
        
        // 通知房间内其他用户
        json notification = {
            {"type", "user_joined"},
            {"user_id", user_id},
            {"room_id", room_id},
            {"message", "User " + user_id + " joined the room"}
        };
        broadcast_to_room(room_id, notification.dump(), user_id); // 排除自己
    }
    catch (const json::exception& e)
    {
        LOG_ERROR << "Error joining room for user " << user_id << ": " << e.what();
        send_error(hdl, "Missing required field: room_id");
    }
}

void WebSocketServer::handle_leave_room(connection_hdl hdl, const std::string& user_id, const json& message)
{
    auto current_room_it = user_current_room_.find(user_id);
    if (current_room_it == user_current_room_.end())
    {
        send_error(hdl, "You are not in any room");
        return;
    }
    
    std::string room_id = current_room_it->second;
    leave_room(user_id, room_id);
    
    LOG_INFO << "User " << user_id << " left room: " << room_id;
    
    // 发送成功响应给用户
    json response = {
        {"type", "room_left"},
        {"room_id", room_id},
        {"message", "Successfully left room"}
    };
    server_.send(hdl, response.dump(), websocketpp::frame::opcode::text);
    
    // 通知房间内其他用户
    json notification = {
        {"type", "user_left"},
        {"user_id", user_id},
        {"room_id", room_id},
        {"message", "User " + user_id + " left the room"}
    };
    broadcast_to_room(room_id, notification.dump());
}

void WebSocketServer::handle_chat_message(connection_hdl hdl, const std::string& user_id, const json& message)
{
    try
    {
        std::string content = message.at("content").get<std::string>();
        
        // 检查用户是否在房间中
        auto current_room_it = user_current_room_.find(user_id);
        if (current_room_it == user_current_room_.end())
        {
            send_error(hdl, "You must join a room before sending messages");
            return;
        }
        
        std::string room_id = current_room_it->second;
        
        // 构造聊天消息
        json chat_msg = {
            {"type", "chat_message"},
            {"user_id", user_id},
            {"room_id", room_id},
            {"content", content},
            {"timestamp", std::time(nullptr)}
        };
        
        // 广播到房间内所有用户（包括发送者）
        broadcast_to_room(room_id, chat_msg.dump());
        
        LOG_INFO << "Chat message from user " << user_id << " in room " << room_id;
    }
    catch (const json::exception& e)
    {
        LOG_ERROR << "Error processing chat message from user " << user_id << ": " << e.what();
        send_error(hdl, "Missing required field: content");
    }
}

void WebSocketServer::join_room(const std::string& user_id, const std::string& room_id)
{
    room_members_[room_id].insert(user_id);
    user_current_room_[user_id] = room_id;
}

void WebSocketServer::leave_room(const std::string& user_id, const std::string& room_id)
{
    auto room_it = room_members_.find(room_id);
    if (room_it != room_members_.end())
    {
        room_it->second.erase(user_id);
        // 如果房间空了，删除房间
        if (room_it->second.empty())
        {
            room_members_.erase(room_it);
        }
    }
    user_current_room_.erase(user_id);
}

void WebSocketServer::send_error(connection_hdl hdl, const std::string& error_message)
{
    json error_response = {
        {"type", "error"},
        {"message", error_message}
    };
    try
    {
        server_.send(hdl, error_response.dump(), websocketpp::frame::opcode::text);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Failed to send error message: " << e.what();
    }
}

void WebSocketServer::broadcast_to_room(const std::string &room_id, const std::string &message)
{
    broadcast_to_room(room_id, message, ""); // 空字符串表示不排除任何用户
}

void WebSocketServer::broadcast_to_room(const std::string &room_id, const std::string &message, const std::string &exclude_user_id)
{
    //std::lock_guard<std::mutex> lock(connection_mutex_);//不能加锁，这里会发送死锁问题
    auto room_it = room_members_.find(room_id);
    if (room_it == room_members_.end())
    {
        LOG_WARN << "Attempted to broadcast to non-existent room: " << room_id;
        return;
    }
    
    int sent_count = 0;
    for (const std::string& user_id : room_it->second)
    {
        if (user_id == exclude_user_id)
            continue; // 跳过被排除的用户
            
        auto conn_it = user_connections_.find(user_id);
        if (conn_it != user_connections_.end())
        {
            try
            {
                server_.send(conn_it->second, message, websocketpp::frame::opcode::text);
                sent_count++;
            }
            catch (const std::exception& e)
            {
                LOG_ERROR << "Failed to send message to user " << user_id << " in room " << room_id << ": " << e.what();
            }
        }
    }
    
    LOG_INFO << "Broadcasted message to " << sent_count << " users in room: " << room_id;
}