#include "http/http_server.hpp"
#include "utils/logger.hpp"
#include "websocket/websocket_server.hpp"
#include "service/auth_service.hpp"
#include "service/room_service.hpp"
#include "service/message_service.hpp"
#include "middleware/auth_middleware.hpp"
#include "db/database_manager.hpp"
#include <iostream>
#include <signal.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <ctime>
#include <cstdlib>

std::atomic<bool> running(true);
std::unique_ptr<WebSocketServer> ws_server;

void signalHandler(int signal)
{
    std::cout << "\n收到信号 " << signal << "，正在关闭服务器..." << std::endl;
    running = false;
}

int main(int argc, char *argv[])
{
    // 设置信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    try
    {
        // 设置JWT密钥环境变量（如果未设置）
        if (!std::getenv("JWT_SECRET")) {
            setenv("JWT_SECRET", "your_secret_key_here", 1);
            LOG_INFO << "JWT_SECRET environment variable set to default value";
        }

        // 初始化数据库管理器
        DatabaseManager db_manager("./db");
        LOG_INFO << "Database manager initialized";

        // 创建HTTP服务器实例
        http::HttpServer server(8080, 4); // 端口8080，4个工作线程

        // 设置静态文件目录 (使用绝对路径)
        server.setStaticDirectory("/home/lbm/SwiftChat/static");

        // 设置日志中间件 - 记录所有请求
        server.setMiddleware(middleware::auth);

        // 初始化服务
        AuthService auth_service(db_manager);
        RoomService room_service(db_manager);
        MessageService message_service(db_manager);
        
        // 注册路由
        auth_service.registerRoutes(server);
        room_service.registerRoutes(server);
        message_service.registerRoutes(server);
        
        LOG_INFO << "All services registered successfully";

        // 创建并启动WebSocket服务器
        ws_server = std::make_unique<WebSocketServer>();
        LOG_INFO << "WebSocket server created";

        // 健康检查接口
        http::HttpServer::Route health_route{
            "/api/health",
            "GET",
            [](const http::HttpRequest &req)
            {
                return http::HttpResponse::Ok(R"({"status": "ok", "message": "Server is running"})");
            },
            false // 不需要认证
        };
        server.addHandler(health_route);

        // 获取服务器信息接口
        http::HttpServer::Route info_route{
            "/api/info",
            "GET",
            [](const http::HttpRequest &req)
            {
                return http::HttpResponse::Ok()
                    .withBody(R"({
                        "name": "SwiftChat HTTP Server",
                        "version": "1.0.0",
                        "description": "A simple HTTP server with WebSocket support"
                    })",
                              "application/json");
            },
            false};
        server.addHandler(info_route);

        // Echo接口 - 返回请求的信息（支持GET和POST）
        http::HttpServer::Route echo_get_route{
            "/api/echo",
            "GET",
            [](const http::HttpRequest &req)
            {
                auto user_agent_opt = req.getHeaderValue("User-Agent");
                std::string user_agent = user_agent_opt.has_value() ? std::string(user_agent_opt.value()) : "Unknown";
                std::string response_body = R"({
                    "method": ")" + req.getMethod() +
                                            R"(",
                    "path": ")" + req.getPath() +
                                            R"(",
                    "message": "Echo GET request received",
                    "user_agent": ")" + user_agent +
                                            R"("
                })";
                return http::HttpResponse::Ok()
                    .withBody(response_body, "application/json");
            },
            false};
        server.addHandler(echo_get_route);

        http::HttpServer::Route echo_post_route{
            "/api/echo",
            "POST",
            [](const http::HttpRequest &req)
            {
                std::string response_body = R"({
                    "method": ")" + req.getMethod() +
                                            R"(",
                    "path": ")" + req.getPath() +
                                            R"(",
                    "received_data": ")" + req.getBody() +
                                            R"("
                })";
                return http::HttpResponse::Ok()
                    .withBody(response_body, "application/json");
            },
            false};
        server.addHandler(echo_post_route);

        // 需要认证的API端点示例
        http::HttpServer::Route protected_route{
            "/api/protected",
            "GET",
            [](const http::HttpRequest &req)
            {
                return http::HttpResponse::Ok()
                    .withBody(R"({
                        "message": "This is a protected endpoint",
                        "data": "Secret information",
                        "timestamp": ")" +
                                  std::to_string(std::time(nullptr)) + R"("
                    })",
                              "application/json");
            },
            true // 需要认证中间件
        };
        server.addHandler(protected_route);

        std::cout << "=== SwiftChat 服务器 ===" << std::endl;
        std::cout << "正在启动服务器..." << std::endl;

        // 在后台线程启动HTTP服务器
        std::thread server_thread([&server]()
                                  { 
                                      LOG_INFO << "HTTP server thread starting...";
                                      server.run(); 
                                  });

        // 在后台线程启动WebSocket服务器
        std::thread websocket_thread([&]()
                                     {
                                         LOG_INFO << "WebSocket server thread starting...";
                                         ws_server->run(8081); // WebSocket运行在8081端口
                                     });

        std::cout << "HTTP服务器已启动: http://localhost:8080" << std::endl;
        std::cout << "WebSocket服务器已启动: ws://localhost:8081" << std::endl;
        std::cout << "\n=== API端点 ===" << std::endl;
        std::cout << "认证API:" << std::endl;
        std::cout << "  - POST /api/v1/auth/register  - 用户注册" << std::endl;
        std::cout << "  - POST /api/v1/auth/login     - 用户登录" << std::endl;
        std::cout << "\n房间API:" << std::endl;
        std::cout << "  - POST /api/v1/rooms          - 创建房间 (需要认证)" << std::endl;
        std::cout << "  - GET  /api/v1/rooms          - 获取房间列表" << std::endl;
        std::cout << "  - POST /api/v1/rooms/join     - 加入房间 (需要认证)" << std::endl;
        std::cout << "\n消息API:" << std::endl;
        std::cout << "  - GET  /api/v1/messages       - 获取房间消息 (需要认证)" << std::endl;
        std::cout << "\n系统API:" << std::endl;
        std::cout << "  - GET  /api/health            - 健康检查" << std::endl;
        std::cout << "  - GET  /api/info              - 服务器信息" << std::endl;
        std::cout << "  - POST /api/echo              - Echo测试" << std::endl;
        std::cout << "  - GET  /api/protected         - 受保护的端点 (需要认证)" << std::endl;
        std::cout << "\n=== WebSocket支持 ===" << std::endl;
        std::cout << "连接: ws://localhost:8081" << std::endl;
        std::cout << "支持的消息类型:" << std::endl;
        std::cout << "  - auth: 用户认证" << std::endl;
        std::cout << "  - join_room: 加入房间" << std::endl;
        std::cout << "  - leave_room: 离开房间" << std::endl;
        std::cout << "  - chat_message: 发送聊天消息" << std::endl;
        std::cout << "  - ping: 心跳检测" << std::endl;
        std::cout << "\n静态文件: /static 目录" << std::endl;
        std::cout << "访问 http://localhost:8080 查看聊天界面" << std::endl;
        std::cout << "按 Ctrl+C 退出" << std::endl;
        std::cout << "================================" << std::endl;

        // 主循环
        while (running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // 停止服务器
        std::cout << "\n正在停止服务器..." << std::endl;
        
        // 停止WebSocket服务器
        if (ws_server) {
            ws_server->stop();
            LOG_INFO << "WebSocket server stopped";
        }
        
        // 停止HTTP服务器
        server.stop();
        LOG_INFO << "HTTP server stopped";

        // 等待服务器线程结束
        if (server_thread.joinable())
        {
            server_thread.join();
        }
        
        if (websocket_thread.joinable())
        {
            websocket_thread.join();
        }

        std::cout << "所有服务器已关闭" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
