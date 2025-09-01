#include <iostream>
#include <string>
#include "http/http_server.hpp"
#include "http/http_response.hpp"
#include "http/http_request.hpp"
#include "utils/logger.hpp"

int main() {
    // 初始化日志系统
    utils::Logger::setGlobalLevel(utils::LogLevel::DEBUG);  // 设置日志级别为DEBUG
    utils::Logger::initFileLogger("./logs/swiftchat.log");   // 启用文件日志
    
    // 设置监听端口
    const int PORT = 8080;

    try {
        // 1. 创建HttpServer实例
        // 使用4个线程来处理请求
        http::HttpServer server(PORT, 4); 

        // 2. 获取路由器实例，并配置路由
        auto& router = server.getRouter();

        // 添加根路径 "/" 的处理器，用于提供HTML测试页面
        router.addHandler({
            "/", "GET",
            [](const http::HttpRequest& req) {
                // 直接返回包含HTML代码的200 OK响应
                return http::HttpResponse::Ok().withBody("index.html", "text/html; charset=utf-8");
            },
            false // 不需要认证
        });

        // (可选) 添加一个简单的HTTP API路由，证明HTTP服务正常工作
        router.addHandler({
            "/api/hello", "GET",
            [](const http::HttpRequest& req) {
                // 返回一个JSON响应
                return http::HttpResponse::Ok().withJsonBody({
                    {"message", "Hello, this is the HTTP API!"}
                });
            },
            false
        });
        

        // 3. 启动服务器
        LOG_INFO << "HTTP and WebSocket server starting...";
        LOG_INFO << "  >> HTTP Test Page: http://localhost:" << PORT;
        LOG_INFO << "  >> WebSocket Endpoint: ws://localhost:" << PORT;
        LOG_INFO << "  >> HTTP API Example: curl http://localhost:" << PORT << "/api/hello";
        
        server.run();

    } catch (const std::exception& e) {
        LOG_ERROR << "Server encountered a fatal error: " << e.what();
        return 1;
    }

    return 0;
}