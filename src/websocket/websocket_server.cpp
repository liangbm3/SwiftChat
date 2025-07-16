#include "websocket_server.hpp"
#include "utils/logger.hpp"
#include <jwt-cpp/jwt.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

WebSocketServer::WebSocketServer()
{
    // 关闭websocketpp的日志
    server_.clear_access_channels(websocketpp::log::alevel::all);
    server_.clear_error_channels(websocketpp::log::elevel::all);

    // 初始化Asio
    server_.init_asio();

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
            //设置监听端口
            server_.listen(port);
            LOG_INFO << "WebSocket server listening on port " << port;

            //开始接受连接
            server_.start_accept();

            //运行Asio事件循环
            server_.run();
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
    if (!server_.is_listening())
    {
        return;
    }
    LOG_INFO << "Stopping WebSocket server...";
    server_.stop_listening(); // 停止监听新连接
    server_.stop();           // 停止IO事件循环，导致run函数返回
    server_thread_.join();    // 等待服务器线程结束
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
                const char *secret_key_cstr = std::getenv("JWT_SECRET_KEY");
                if (!secret_key_cstr)
                {
                    LOG_ERROR << "FATAL: JWT_SECRET_KEY not set for verification.";
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
                    {"message", "Authentication successful"}};
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
            server_.close(hdl, websocketpp::close::status::policy_violation, "Invalid token");
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "WebSocket message handling error: " << e.what();
            server_.close(hdl, websocketpp::close::status::internal_endpoint_error, "Internal server error");
        }
    }
}

void WebSocketServer::send_to_user(const std::string &user_id, const std::string &message)
{
    std::lock_guard<std::mutex> lock(connection_mutex_);
    auto it = user_connections_.find(user_id);
    if (it != user_connections_.end())
    {
        try
        {
            server_.send(it->second, message, websocketpp::frame::opcode::text);
            LOG_INFO << "Message sent to user: " << user_id;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "Failed to send message to user " << user_id << ": " << e.what();
        }
    }
    else
    {
        LOG_WARN << "User " << user_id << " not found in active connections";
    }
}