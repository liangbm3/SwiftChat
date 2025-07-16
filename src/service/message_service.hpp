#pragma once

#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "http/http_server.hpp"
#include <optional>

class DatabaseManager;

class MessageService
{
public:
    explicit MessageService(DatabaseManager &db_manager);
    ~MessageService() = default;

    void registerRoutes(http::HttpServer &server);
private:
    DatabaseManager &db_manager_; // 数据库管理器引用
    http::HttpResponse getMessages(const http::HttpRequest &request); // 获取消息列表
    std::optional<std::string> getUserIdFromRequest(const http::HttpRequest& request);
};