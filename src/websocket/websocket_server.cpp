#include "websocket_server.hpp"
#include "utils/logger.hpp"
#include "utils/jwt_utils.hpp"
#include "db/database_manager.hpp"
#include <nlohmann/json.hpp>
#include <ctime>

using json = nlohmann::json;

WebSocketServer::WebSocketServer(DatabaseManager &db_manager)
    : db_manager_(db_manager)
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

    try
    {
        // 停止监听新连接
        if (server_.is_listening())
        {
            websocketpp::lib::error_code ec;
            server_.stop_listening(ec);
            if (ec)
            {
                LOG_ERROR << "Error stopping listening: " << ec.message();
            }
        }

        // 关闭所有现有连接
        {
            std::lock_guard<std::mutex> lock(connection_mutex_);
            for (const auto &pair : connection_users_)
            {
                websocketpp::lib::error_code ec;
                server_.close(pair.first, websocketpp::close::status::going_away, "Server shutdown", ec);
                if (ec)
                {
                    LOG_ERROR << "Error closing connection: " << ec.message();
                }
            }
        }

        // 停止IO事件循环
        server_.stop();

        // 等待服务器线程结束
        if (server_thread_.joinable())
        {
            server_thread_.join();
        }

        LOG_INFO << "WebSocket server stopped successfully";
    }
    catch (const std::exception &e)
    {
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

        // 如果用户在房间中，先通知其他用户再移除
        auto room_it = user_current_room_.find(user_id);
        if (room_it != user_current_room_.end())
        {
            std::string room_id = room_it->second;

            // 获取用户信息
            auto user_info = db_manager_.getUserById(user_id);
            std::string username = user_info ? user_info->getUsername() : user_id;

            // 通知房间内其他用户该用户已离开
            json notification = {
                {"success", true},
                {"message", "User left room"},
                {"data", {{"type", "user_left"}, {"user_id", user_id}, {"username", username}, {"room_id", room_id}}}};
            broadcast_to_room(room_id, notification.dump(), user_id); // 排除自己

            // 然后从房间中移除用户
            leave_room(user_id, room_id);
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
    // 先检查连接是否验证
    std::string user_id;
    bool is_authenticated = false;
    {
        // 使用互斥锁保护共享数据
        std::lock_guard<std::mutex> lock(connection_mutex_);
        // 检查连接是否已经认证
        auto it = connection_users_.find(hdl);
        if (it != connection_users_.end())
        {
            is_authenticated = true;
            user_id = it->second; // 线程安全地获取用户id
        }
    }

    if (!is_authenticated) // 处理未认证的连接，期望收到的认证消息
    {
        try
        {
            auto json_msg = json::parse(msg->get_payload());
            if (json_msg.value("type", "") == "auth")
            {
                // 处理认证消息 - 只需要token
                std::string token = json_msg.at("token").get<std::string>();

                // 验证JWT令牌并获取用户ID
                auto verified_user_id = JwtUtils::verifyToken(token);
                if (!verified_user_id)
                {
                    LOG_ERROR << "JWT verification failed";
                    json error_response = {
                        {"success", false},
                        {"message", "Authentication failed"},
                        {"error", "Invalid or expired token"}};
                    server_.send(hdl, error_response.dump(), websocketpp::frame::opcode::text);
                    server_.close(hdl, websocketpp::close::status::policy_violation, "Invalid token");
                    return;
                }

                std::string verified_id = *verified_user_id;

                { // 进入临界区
                    std::lock_guard<std::mutex> lock(connection_mutex_);
                    // 检查用户是否已有连接
                    auto old_connection_it = user_connections_.find(verified_id);
                    if (old_connection_it != user_connections_.end())
                    {
                        LOG_INFO << "User " << verified_id << " already has a connection. Closing old connection.";
                        // 获取旧连接句柄
                        // 向旧连接发送通知
                        json reason = {
                            {"success", false},
                            {"message", "Connection closed due to new login"},
                            {"error", "logged_in_from_another_location"}};
                        try
                        {
                            server_.close(old_connection_it->second, websocketpp::close::status::policy_violation, reason.dump());
                        }
                        catch (const std::exception &e)
                        {
                            LOG_ERROR << "Error closing old connection for user " << verified_id << ": " << e.what();
                        }

                        // 关闭旧连接
                        // 从反向映射中移除旧的句柄记录
                        // on_close处理器之后也会做这件事，但在这里提前做可以保证状态立即更新
                        connection_users_.erase(old_connection_it->second);
                    }
                    // 认证通过，保存连接和用户ID映射
                    user_connections_[verified_id] = hdl;
                    connection_users_[hdl] = verified_id;
                }

                LOG_INFO << "WebSocket connection authenticated for user: " << verified_id;
                json response = {
                    {"success", true},
                    {"message", "WebSocket authentication successful"},
                    {"data", {{"user_id", verified_id}, {"status", "connected"}}}};
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
        catch (const std::exception &e)
        {
            LOG_ERROR << "WebSocket message handling error: " << e.what();
            json error_response = {
                {"success", false},
                {"message", "Internal server error"},
                {"error", "Failed to process authentication"}};
            server_.send(hdl, error_response.dump(), websocketpp::frame::opcode::text);
            server_.close(hdl, websocketpp::close::status::internal_endpoint_error, "Internal server error");
        }
    }
    else
    {
        // 处理已认证用户的消息
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

void WebSocketServer::handle_authenticated_message(connection_hdl hdl, const std::string &user_id, const json &message)
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
    else if (msg_type == "send_message")
    {
        handle_chat_message(hdl, user_id, message);
    }
    else if (msg_type == "ping")
    {
        // 处理心跳消息
        json pong_response = {
            {"success", true},
            {"message", "Pong response"},
            {"data", {{"type", "pong"}, {"timestamp", std::time(nullptr)}}}};
        server_.send(hdl, pong_response.dump(), websocketpp::frame::opcode::text);
    }
    else
    {
        LOG_WARN << "Unknown message type '" << msg_type << "' from user: " << user_id;
        send_error(hdl, "Unknown message type: " + msg_type);
    }
}

void WebSocketServer::handle_join_room(connection_hdl hdl, const std::string &user_id, const json &message)
{
    try
    {
        std::string room_id = message.at("room_id").get<std::string>();

        std::lock_guard<std::mutex> lock(connection_mutex_); // 访问共享数据，进入临界区
        // 检查用户是否在房间中
        auto current_room_it = user_current_room_.find(user_id);
        if (current_room_it != user_current_room_.end() && current_room_it->second == room_id)
        {
            LOG_WARN << "User " << user_id << " tried to join room " << room_id << " but is already in it.";
            send_error(hdl, "You are already in this room");
            return;
        }
        // 如果用户已经在其他房间，先通知原房间其他用户然后离开
        if (current_room_it != user_current_room_.end())
        {
            std::string old_room_id = current_room_it->second;

            // 获取用户信息
            auto user_info = db_manager_.getUserById(user_id);
            std::string username = user_info ? user_info->getUsername() : user_id;

            // 通知原房间内其他用户该用户已离开
            json leave_notification = {
                {"success", true},
                {"message", "User left room"},
                {"data", {{"type", "user_left"}, {"user_id", user_id}, {"username", username}, {"room_id", old_room_id}}}};
            broadcast_to_room(old_room_id, leave_notification.dump(), user_id); // 排除自己

            // 然后从原房间移除用户
            leave_room(user_id, old_room_id);
        }

        // 加入新房间
        join_room(user_id, room_id);

        LOG_INFO << "User " << user_id << " joined room: " << room_id;

        // 发送成功响应给用户
        json response = {
            {"success", true},
            {"message", "Room joined successfully"},
            {"data", {{"type", "room_joined"}, {"room_id", room_id}, {"user_id", user_id}}}};
        server_.send(hdl, response.dump(), websocketpp::frame::opcode::text);

        // 获取用户信息
        auto user_info = db_manager_.getUserById(user_id);
        std::string username = user_info ? user_info->getUsername() : user_id;

        // 通知房间内其他用户
        json notification = {
            {"success", true},
            {"message", "User joined room"},
            {"data", {{"type", "user_joined"}, {"user_id", user_id}, {"username", username}, {"room_id", room_id}}}};
        broadcast_to_room(room_id, notification.dump(), user_id); // 排除自己
    }
    catch (const json::exception &e)
    {
        LOG_ERROR << "Error joining room for user " << user_id << ": " << e.what();
        send_error(hdl, "Missing required field: room_id");
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Exception joining room for user " << user_id << ": " << e.what();
        send_error(hdl, "Failed to join room");
    }
}

void WebSocketServer::handle_leave_room(connection_hdl hdl, const std::string &user_id, const json &message)
{
    std::lock_guard<std::mutex> lock(connection_mutex_);
    auto current_room_it = user_current_room_.find(user_id);
    if (current_room_it == user_current_room_.end())
    {
        send_error(hdl, "You are not in any room");
        return;
    }

    std::string room_id = current_room_it->second;

    LOG_INFO << "User " << user_id << " left room: " << room_id;

    // 获取用户信息
    auto user_info = db_manager_.getUserById(user_id);
    std::string username = user_info ? user_info->getUsername() : user_id;

    // 先通知房间内其他用户，再移除当前用户
    json notification = {
        {"success", true},
        {"message", "User left room"},
        {"data", {{"type", "user_left"}, {"user_id", user_id}, {"username", username}, {"room_id", room_id}}}};
    broadcast_to_room(room_id, notification.dump(), user_id); // 排除自己

    // 然后移除用户
    leave_room(user_id, room_id);

    // 发送成功响应给用户
    json response = {
        {"success", true},
        {"message", "Room left successfully"},
        {"data", {{"type", "room_left"}, {"room_id", room_id}, {"user_id", user_id}}}};
    server_.send(hdl, response.dump(), websocketpp::frame::opcode::text);
}

void WebSocketServer::handle_chat_message(connection_hdl hdl, const std::string &user_id, const json &message)
{
    try
    {
        int64_t timestamp = std::time(nullptr); // 只生成一次时间戳
        std::string content = message.at("content").get<std::string>();
        std::string room_id;

        { // 临界区开始
            std::lock_guard<std::mutex> lock(connection_mutex_);
            // 检查用户是否在房间中
            auto current_room_it = user_current_room_.find(user_id);
            if (current_room_it == user_current_room_.end())
            {
                send_error(hdl, "You must join a room before sending messages");
                return;
            }
            room_id = current_room_it->second;
        } // 锁释放

        // 保存消息到数据库
        try
        {
            db_manager_.saveMessage(room_id, user_id, content, timestamp);
            LOG_INFO << "Message saved to database from user " << user_id << " in room " << room_id;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "Failed to save message to database: " << e.what();
            send_error(hdl, "Failed to save message");
            return;
        }

        // 获取用户信息
        auto user_info = db_manager_.getUserById(user_id);
        std::string username = user_info ? user_info->getUsername() : user_id;

        // 构造聊天消息
        json chat_msg = {
            {"success", true},
            {"message", "Message sent successfully"},
            {"data", {{"type", "message_received"}, {"user_id", user_id}, {"username", username}, {"room_id", room_id}, {"content", content}, {"timestamp", timestamp}}}};

        // 广播到房间内所有用户（包括发送者）
        broadcast_to_room(room_id, chat_msg.dump());

        LOG_INFO << "Chat message from user " << user_id << " in room " << room_id;
    }
    catch (const json::exception &e)
    {
        LOG_ERROR << "Error processing chat message from user " << user_id << ": " << e.what();
        send_error(hdl, "Missing required field: content");
    }
}

void WebSocketServer::join_room(const std::string &user_id, const std::string &room_id)
{
    room_members_[room_id].insert(user_id);
    user_current_room_[user_id] = room_id;
}

void WebSocketServer::leave_room(const std::string &user_id, const std::string &room_id)
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

void WebSocketServer::send_error(connection_hdl hdl, const std::string &error_message)
{
    json error_response = {
        {"success", false},
        {"message", "Request failed"},
        {"error", error_message}};
    try
    {
        server_.send(hdl, error_response.dump(), websocketpp::frame::opcode::text);
    }
    catch (const std::exception &e)
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
    std::vector<connection_hdl> connections_to_send; // 需要发送的连接
    {
        // // 加上锁，安全地访问共享数据
        // std::lock_guard<std::mutex> lock(connection_mutex_);
        auto room_it = room_members_.find(room_id);
        if (room_it == room_members_.end())
        {
            LOG_WARN << "Attempted to broadcast to non-existent or empty room: " << room_id;
            return;
        }

        // 创建一个需要发送的连接的“快照”
        for (const std::string &user_id : room_it->second)
        {
            if (user_id == exclude_user_id)
            {
                continue;
            }

            auto conn_it = user_connections_.find(user_id);
            if (conn_it != user_connections_.end())
            {
                connections_to_send.push_back(conn_it->second);
            }
        }
    } // 锁在这里被释放

    LOG_INFO << "Broadcasting message to " << connections_to_send.size() << " users in room: " << room_id;

    // 在不持有锁的情况下执行发送操作
    for (const auto &hdl : connections_to_send)
    {
        try
        {
            server_.send(hdl, message, websocketpp::frame::opcode::text);
        }
        catch (const websocketpp::exception &e)
        {
            // 在快照和发送的间隙，用户可能已经断开连接了，这是正常情况
            LOG_ERROR << "Failed to send message during broadcast: " << e.what();
        }
    }
}