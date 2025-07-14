#pragma once

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
    http::HttpResponse handleCreateRoom(const http::HttpRequest &request);
    http::HttpResponse handleJoinRoom(const http::HttpRequest &request);
    http::HttpResponse handleGetRooms(const http::HttpRequest &request);
    
    DatabaseManager &db_manager_; // 数据库管理器引用
};