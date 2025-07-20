#pragma once

#include "../http/http_server.hpp"
#include "../http/http_request.hpp"
#include "../http/http_response.hpp"
#include "../db/database_manager.hpp"

class ServerService {
public:
    explicit ServerService(DatabaseManager& db_manager);
    
    void registerRoutes(http::HttpServer& server);

private:
    DatabaseManager& db_manager_;
    
    // 服务器相关的API处理方法
    http::HttpResponse handleHealthCheck(const http::HttpRequest& req);
    http::HttpResponse handleServerInfo(const http::HttpRequest& req);
    http::HttpResponse handleEchoGet(const http::HttpRequest& req);
    http::HttpResponse handleEchoPost(const http::HttpRequest& req);
    http::HttpResponse handleProtected(const http::HttpRequest& req);
    
    // 服务器版本和信息
    static constexpr const char* SERVER_NAME = "SwiftChat HTTP Server";
    static constexpr const char* SERVER_VERSION = "1.0.0";
    static constexpr const char* SERVER_DESCRIPTION = "A simple HTTP server with WebSocket support";
};
