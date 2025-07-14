#include "room_service.hpp"
#include "db/database_manager.hpp"
#include "http/http_server.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "utils/logger.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

RoomService::RoomService(DatabaseManager &db_manager) : db_manager_(db_manager) {}

void RoomService::registerRoutes(http::HttpServer &server)
{
    // 注册房间相关的HTTP路由
    server.addHandler("/api/rooms/create", "POST", [this](const http::HttpRequest &request) -> http::HttpResponse
                      { return handleCreateRoom(request); });

    server.addHandler("/api/rooms/join", "POST", [this](const http::HttpRequest &request) -> http::HttpResponse
                      { return handleJoinRoom(request); });

    server.addHandler("/api/rooms/list", "GET", [this](const http::HttpRequest &request) -> http::HttpResponse
                      { return handleGetRooms(request); });
}

http::HttpResponse RoomService::handleCreateRoom(const http::HttpRequest &request)
{
    // 这里需要检查用户是否已登录 (通过JWT或Session) !!!
    LOG_INFO << "Handling create room request...";
    try
    {
        if (request.body.empty())
        {
            LOG_ERROR << "Request body is empty";
            return http::HttpResponse(400, "{\"error\":\"Request body is empty\"}");
        }
        json request_json = json::parse(request.body);
        std::string room_name = request_json.at("name").get<std::string>();
        std::string creator = request_json.at("creator").get<std::string>();
        if (room_name.empty() || creator.empty())
        {
            LOG_ERROR << "Room name and creator cannot be empty";
            return http::HttpResponse(400, "{\"error\":\"Room name and creator cannot be empty\"}");
        }
        if (db_manager_.roomExists(room_name))
        {
            LOG_ERROR << "Room already exists: " << room_name;
            return http::HttpResponse(400, "{\"error\":\"Room already exists\"}");
        }
        if (db_manager_.createRoom(room_name, creator))
        {
            // 创建成功后，自动加入房间
            db_manager_.addRoomMember(room_name, creator);
            LOG_INFO << "Room created successfully: " << room_name << " by " << creator;
            return http::HttpResponse(201, "{\"status\":\"success\",\"message\":\"Room created successfully\"}");
        }
        else
        {
            LOG_ERROR << "Failed to create room: " << room_name;
            return http::HttpResponse(500, "{\"error\":\"Failed to create room\"}");
        }
    }
    catch (const json::exception &e)
    {
        LOG_ERROR << "JSON parsing error: " << e.what();
        return http::HttpResponse(400, "{\"error\":\"Invalid JSON format\"}");
    }
}

http::HttpResponse RoomService::handleJoinRoom(const http::HttpRequest &request)
{
    // 这里需要检查用户是否已登录 (通过JWT或Session) !!!
    LOG_INFO << "Handling join room request...";
    try
    {
        json data = json::parse(request.body);
        if (!data.contains("room_name") || !data.contains("username"))
        {
            LOG_ERROR << "Missing room_name or username in join_room request";
            return http::HttpResponse(400, "{\"error\":\"Missing room_name or username\"}");
        }
        std::string room_name = data["room_name"];
        std::string username = data["username"];

        if (db_manager_.addRoomMember(room_name, username))
        {
            LOG_INFO << "User " << username << " joined room: " << room_name;
            return http::HttpResponse(200, "{\"status\":\"success\",\"message\":\"Joined room successfully\"}");
        }
        else
        {
            LOG_ERROR << "Failed to add user to room: " << room_name;
            return http::HttpResponse(500, "{\"error\":\"Failed to join room\"}");
        }
    }
    catch (const json::exception &e)
    {
        LOG_ERROR << "JSON parsing error: " << e.what();
        return http::HttpResponse(400, "{\"error\":\"Invalid JSON format\"}");
    }
}

http::HttpResponse RoomService::handleGetRooms(const http::HttpRequest &request)
{
    LOG_INFO << "Handling get rooms request...";
    auto rooms = db_manager_.getRooms();
    json response_data = json::array();
    for (const auto &room : rooms)
    {
        json room_data =
            {
                {"name", room},
                {"members", db_manager_.getRoomMembers(room)}};
        response_data.push_back(room_data);
    }
    return http::HttpResponse(200, response_data.dump());
}
