#include "room_service.hpp"
#include "db/database_manager.hpp"
#include "http/http_server.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "utils/logger.hpp"
#include <nlohmann/json.hpp>
#include <jwt-cpp/jwt.h>

using json = nlohmann::json;

RoomService::RoomService(DatabaseManager &db_manager) : db_manager_(db_manager) {}

std::optional<std::string> RoomService::getUserIdFromRequest(const http::HttpRequest &request)
{
    auto auth_header = request.getHeaderValue("Authorization");
    if(!auth_header)
    {
        LOG_ERROR << "Authorization header is missing in the request.";
        return std::nullopt;
    }

    std::string token_str = std::string(auth_header->substr(7)); // 去掉 "Bearer " 前缀
    try
    {
        auto decoded_token = jwt::decode(token_str);
        return decoded_token.get_subject(); // 使用 subject 声明获取用户ID
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << "Failed to decode JWT: " << e.what();
        return std::nullopt;
    }
}

http::HttpResponse RoomService::handleCreateRoom(const http::HttpRequest &request)
{
    auto user_id_opt = getUserIdFromRequest(request);//获取创建者ID
    if (!user_id_opt)
    {
        LOG_ERROR << "Failed to get user ID from request.";
        return http::HttpResponse::Unauthorized("Invalid or missing JWT token.");
    }
    
    std::string user_id = *user_id_opt;

    json request_body;
    try
    {
        request_body = json::parse(request.getBody());
        std::string room_name = request_body.at("name").get<std::string>();//尝试获取房间名
        //调用DB接口创建房间
        auto room_json_opt = db_manager_.createRoom(room_name, user_id);
        if (!room_json_opt)
        {
            LOG_ERROR << "Failed to create room for user: " << user_id;
            return http::HttpResponse::InternalError("Failed to create room.");
        }

        //创建者自动加入房间
        std::string room_id = room_json_opt->at("id").get<std::string>();
        db_manager_.addRoomMember(room_id, user_id);

        return http::HttpResponse::Created().withJsonBody(*room_json_opt);
    }
    catch (const json::parse_error &e)
    {
        LOG_ERROR << "Failed to parse JSON body: " << e.what();
        return http::HttpResponse::BadRequest("Invalid JSON format.");
    }
    catch(const json::out_of_range &e)
    {
        LOG_ERROR << "Missing required fields in JSON body: " << e.what();
        return http::HttpResponse::BadRequest("Missing required fields in JSON body.");
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Unexpected error while creating room: " << e.what();
        return http::HttpResponse::InternalError("Unexpected error occurred.");
    }
}

http::HttpResponse RoomService::handleJoinRoom(const http::HttpRequest &request)
{
    // 获取当前用户的ID
    auto user_id_opt = getUserIdFromRequest(request);
    if (!user_id_opt)
    {
        LOG_ERROR << "Failed to get user ID from request.";
        return http::HttpResponse::Unauthorized("Invalid or missing JWT token.");
    }

    std::string user_id = *user_id_opt;

    try
    {
        // 从请求体中获取房间ID
        auto json_body = json::parse(request.getBody());
        std::string room_id = json_body.value("room_id", "");
        
        //检查房间和用户是否存在
        if(!db_manager_.roomExists(room_id)||!db_manager_.userExists(user_id))
        {
            LOG_ERROR << "Room or user does not exist. Room ID: " << room_id << ", User ID: " << user_id;
            return http::HttpResponse::NotFound("Room or user does not exist.");
        }

        //将用户加入房间
        if (!db_manager_.addRoomMember(room_id, user_id))
        {
            LOG_ERROR << "Failed to add user to room. Room ID: " << room_id << ", User ID: " << user_id;
            return http::HttpResponse::InternalError("Failed to join room.");
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << "Failed to get rooms for user: " << user_id << ", Error: " << e.what();
        return http::HttpResponse::InternalError("Failed to get rooms.");
    }
    catch(const json::parse_error &e)
    {
        LOG_ERROR << "Failed to parse JSON body: " << e.what();
        return http::HttpResponse::BadRequest("Invalid JSON format.");
    }
    catch(const json::out_of_range &e)
    {
        LOG_ERROR << "Missing required fields in JSON body: " << e.what();
        return http::HttpResponse::BadRequest("Missing required fields in JSON body.");
    }
    
}

http::HttpResponse RoomService::handleGetRooms(const http::HttpRequest &request)
{
    try
    {
        auto rooms=db_manager_.getRooms();
        json json_response = rooms;
        return http::HttpResponse::Ok().withJsonBody(json_response);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << "Failed to get rooms: " << e.what();
        return http::HttpResponse::InternalError("Failed to get rooms.");
    }
    
}