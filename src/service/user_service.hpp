#pragma once

#include <string>
#include <optional>
#include <memory>

// 前向声明
namespace http {
    class HttpServer;
    class HttpRequest;
    class HttpResponse;
}

class DatabaseManager;
class UserStatusManager;

class UserService
{
public:
    explicit UserService(DatabaseManager &db_manager);
    
    // 设置用户状态管理器
    void setStatusManager(std::shared_ptr<UserStatusManager> status_manager);
    
    ~UserService() = default;

    void registerRoutes(http::HttpServer &server);

private:
    // 用户信息管理
    http::HttpResponse handleGetCurrentUser(const http::HttpRequest &request);  // 获取当前用户信息
    http::HttpResponse handleGetAllUsers(const http::HttpRequest &request);     // 获取所有用户列表
    http::HttpResponse handleGetUserById(const http::HttpRequest &request);     // 获取指定用户信息
    http::HttpResponse handleGetOnlineUsers(const http::HttpRequest &request);  // 获取在线用户列表
    http::HttpResponse handleGetUserStatus(const http::HttpRequest &request);   // 获取用户状态
    
    // 私有辅助函数，用于从请求中安全地提取用户ID
    std::optional<std::string> getUserIdFromRequest(const http::HttpRequest& request);

    DatabaseManager &db_manager_; // 数据库管理器引用
    std::shared_ptr<UserStatusManager> status_manager_;
};
