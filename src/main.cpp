#include "http/http_server.hpp"
#include "utils/logger.hpp"
#include <iostream>
#include <signal.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <ctime>
#include "service/auth_service.hpp"
#include "middleware/auth_middleware.hpp"
#include "db/database_manager.hpp"

std::atomic<bool> running(true);

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
        DatabaseManager db_manager("./db");
        // 创建HTTP服务器实例
        http::HttpServer server(8080, 4); // 端口8080，4个工作线程

        // 设置静态文件目录
        server.setStaticDirectory("static");

        // 设置日志中间件 - 记录所有请求
        server.setMiddleware(middleware::auth);

        AuthService auth_service(db_manager);
        auth_service.registerRoutes(server);

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

        std::cout << "=== SwiftChat HTTP 服务器 ===" << std::endl;
        std::cout << "正在启动 HTTP 服务器..." << std::endl;

        // 在后台线程启动HTTP服务器
        std::thread server_thread([&server]()
                                  { server.run(); });

        std::cout << "HTTP服务器已启动: http://localhost:8080" << std::endl;
        std::cout << "API端点:" << std::endl;
        std::cout << "  - GET  /api/health    - 健康检查" << std::endl;
        std::cout << "  - GET  /api/info      - 服务器信息" << std::endl;
        std::cout << "  - POST /api/echo      - Echo测试" << std::endl;
        std::cout << "  - GET  /api/protected - 受保护的端点 (需要认证)" << std::endl;
        std::cout << "静态文件: /static 目录" << std::endl;
        std::cout << "访问 http://localhost:8080 查看静态文件" << std::endl;
        std::cout << "中间件功能:" << std::endl;
        std::cout << "  - 请求日志记录" << std::endl;
        std::cout << "  - 处理时间统计" << std::endl;
        std::cout << "  - 自定义响应头 (X-Processing-Time, X-Server)" << std::endl;
        std::cout << "按 Ctrl+C 退出" << std::endl;
        std::cout << "================================" << std::endl;

        // 主循环
        while (running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // 停止服务器
        std::cout << "正在停止 HTTP 服务器..." << std::endl;
        server.stop();

        // 等待服务器线程结束
        if (server_thread.joinable())
        {
            server_thread.join();
        }

        std::cout << "HTTP 服务器已关闭" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
