#pragma once

//前向声明
namespace http
{
    class HttpServer;
    class HttpRequest;
    class HttpResponse;
}

class DatabaseManager;

class UserService
{
public:
    explicit UserService(DatabaseManager& db_manager);
    ~UserService()=default;
    void registerRoutes(http::HttpServer& server);
private:
    DatabaseManager& db_manager_; // 数据库管理器引用
    // 处理用户注册请求
    http::HttpResponse handleRegister(const http::HttpRequest &request);
    // 处理用户登录请求
    http::HttpResponse handleLogin(const http::HttpRequest &request);
};