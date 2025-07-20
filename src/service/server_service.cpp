#include "server_service.hpp"
#include "../utils/logger.hpp"
#include <nlohmann/json.hpp>
#include <ctime>

using json = nlohmann::json;

ServerService::ServerService(DatabaseManager& db_manager) 
    : db_manager_(db_manager) {
    LOG_INFO << "ServerService initialized";
}

void ServerService::registerRoutes(http::HttpServer& server) {
    // 健康检查接口
    http::HttpServer::Route health_route{
        "/api/v1/health",
        "GET",
        [this](const http::HttpRequest& req) {
            return this->handleHealthCheck(req);
        },
        false // 不需要认证
    };
    server.addHandler(health_route);

    // 获取服务器信息接口
    http::HttpServer::Route info_route{
        "/api/v1/info",
        "GET",
        [this](const http::HttpRequest& req) {
            return this->handleServerInfo(req);
        },
        false // 不需要认证
    };
    server.addHandler(info_route);

    // Echo接口 - GET
    http::HttpServer::Route echo_get_route{
        "/api/v1/echo",
        "GET",
        [this](const http::HttpRequest& req) {
            return this->handleEchoGet(req);
        },
        false // 不需要认证
    };
    server.addHandler(echo_get_route);

    // Echo接口 - POST
    http::HttpServer::Route echo_post_route{
        "/api/v1/echo",
        "POST",
        [this](const http::HttpRequest& req) {
            return this->handleEchoPost(req);
        },
        false // 不需要认证
    };
    server.addHandler(echo_post_route);

    // 需要认证的API端点示例
    http::HttpServer::Route protected_route{
        "/api/v1/protected",
        "GET",
        [this](const http::HttpRequest& req) {
            return this->handleProtected(req);
        },
        true // 需要认证中间件
    };
    server.addHandler(protected_route);

    LOG_INFO << "ServerService routes registered successfully";
}

http::HttpResponse ServerService::handleHealthCheck(const http::HttpRequest& req) {
    json response = {
        {"success", true},
        {"message", "Server is running"},
        {"data", {
            {"status", "ok"},
            {"timestamp", std::time(nullptr)},
            {"uptime", "unknown"} // 可以后续添加服务器启动时间计算
        }}
    };
    
    return http::HttpResponse::Ok(response.dump());
}

http::HttpResponse ServerService::handleServerInfo(const http::HttpRequest& req) {
    json response = {
        {"success", true},
        {"message", "Server information retrieved successfully"},
        {"data", {
            {"name", SERVER_NAME},
            {"version", SERVER_VERSION},
            {"description", SERVER_DESCRIPTION},
            {"timestamp", std::time(nullptr)}
        }}
    };
    
    return http::HttpResponse::Ok()
        .withBody(response.dump(), "application/json");
}

http::HttpResponse ServerService::handleEchoGet(const http::HttpRequest& req) {
    auto user_agent_opt = req.getHeaderValue("User-Agent");
    std::string user_agent = user_agent_opt.has_value() ? std::string(user_agent_opt.value()) : "Unknown";
    
    json response = {
        {"success", true},
        {"message", "Echo GET request received"},
        {"data", {
            {"method", req.getMethod()},
            {"path", req.getPath()},
            {"user_agent", user_agent},
            {"timestamp", std::time(nullptr)}
        }}
    };
    
    return http::HttpResponse::Ok()
        .withBody(response.dump(), "application/json");
}

http::HttpResponse ServerService::handleEchoPost(const http::HttpRequest& req) {
    json response = {
        {"success", true},
        {"message", "Echo POST request received"},
        {"data", {
            {"method", req.getMethod()},
            {"path", req.getPath()},
            {"received_data", req.getBody()},
            {"timestamp", std::time(nullptr)}
        }}
    };
    
    return http::HttpResponse::Ok()
        .withBody(response.dump(), "application/json");
}

http::HttpResponse ServerService::handleProtected(const http::HttpRequest& req) {
    json response = {
        {"success", true},
        {"message", "This is a protected endpoint"},
        {"data", {
            {"secret_info", "Secret information"},
            {"timestamp", std::time(nullptr)},
            {"access_level", "authenticated"}
        }}
    };
    
    return http::HttpResponse::Ok()
        .withBody(response.dump(), "application/json");
}
