#pragma once

#include "http/http_request.hpp"
#include "http/http_response.hpp"

class DatabaseManager;

class AuthService
{
    explicit AuthService(DatabaseManager& db_manager);
    //处理用户注册请求
    http::HttpResponse registerUser(const http::HttpRequest& request);
    //处理用户登录请求
    http::HttpResponse loginUser(const http::HttpRequest& request);
private:
    DatabaseManager& db_manager_; // 数据库管理器引用，用于与数据库交互
    std::string hashPassword(const std::string& password); // 对明文密码进行哈希的方法
    http::HttpResponse AuthService::createAndSignToken(const User& user); // 创建并签名JWT令牌的方法
};