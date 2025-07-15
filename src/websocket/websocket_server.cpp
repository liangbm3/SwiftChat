#include "websocket_server.hpp"
#include "utils/logger.hpp"

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
    LOG_INFO << "WebSocket connection closed";
    // 可以在这里处理连接关闭的清理逻辑
}

void WebSocketServer::on_message(connection_hdl hdl, websocket_server::message_ptr msg)
{
    // 暂时只打印收到的消息，并将其原样发回（Echo服务）
    LOG_INFO << "Received WebSocket message: " << msg->get_payload();
    try
    {
        server_.send(hdl, msg->get_payload(), msg->get_opcode());
    }
    catch (const websocketpp::exception &e)
    {
        LOG_ERROR << "Failed to send message: " << e.what();
    }
}