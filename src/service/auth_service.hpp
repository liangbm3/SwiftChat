#pragma once

#include <string>
#include <memory>

// 前向声明
namespace http {
    class HttpServer;
    class HttpRequest;
    class HttpResponse;
}

class DatabaseManager;
class User;

class AuthService
{
public:
    explicit AuthService(DatabaseManager& db_manager);
    
    void registerRoutes(http::HttpServer &server);
    
private:
    DatabaseManager& db_manager_; // 数据库管理器引用，用于与数据库交互
    
    //处理用户注册请求
    http::HttpResponse registerUser(const http::HttpRequest& request);
    //处理用户登录请求
    http::HttpResponse loginUser(const http::HttpRequest& request);
    //处理用户注销请求
    http::HttpResponse logoutUser(const http::HttpRequest& request);
    std::string hashPassword(const std::string& password); // 对明文密码进行哈希的方法
    http::HttpResponse createAndSignToken(const User& user, bool is_registration = false); // 创建并签名JWT令牌的方法
};