#pragma once

#include <string>
#include <optional>

// 前向声明
namespace http {
    class HttpServer;
    class HttpRequest;
    class HttpResponse;
}

class DatabaseManager;

class RoomService
{
public:
    explicit RoomService(DatabaseManager &db_manager);
    ~RoomService() = default;

    void registerRoutes(http::HttpServer &server);
private:
    // 房间管理
    http::HttpResponse handleCreateRoom(const http::HttpRequest &request);//创建房间，需要验证
    http::HttpResponse handleGetRooms(const http::HttpRequest &request);//获取房间列表，不需要验证
    
    // 房间成员管理
    http::HttpResponse handleJoinRoom(const http::HttpRequest &request);//加入房间，需要验证
    
    // 私有辅助函数，用于从请求中安全地提取用户ID
    std::optional<std::string> getUserIdFromRequest(const http::HttpRequest& request);

    DatabaseManager &db_manager_; // 数据库管理器引用
};