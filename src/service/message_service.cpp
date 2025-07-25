#include "message_service.hpp"
#include "db/database_manager.hpp"
#include "utils/jwt_utils.hpp"
#include <nlohmann/json.hpp>
#include "utils/logger.hpp"
#include <charconv>
#include <string>
#include <algorithm>
using json = nlohmann::json;

MessageService::MessageService(DatabaseManager &db_manager) : db_manager_(db_manager) {}

void MessageService::registerRoutes(http::HttpServer &server)
{
    http::HttpServer::Route route{
        .path = "/api/v1/messages",
        .method = "GET",
        .handler = [this](const http::HttpRequest &request) {
            return getMessages(request);
        },
        .use_auth_middleware = true // 使用认证中间件
    };
    server.addHandler(route);
}

std::optional<std::string> MessageService::getUserIdFromRequest(const http::HttpRequest &request)
{
    return JwtUtils::getUserIdFromRequest(request);
}

// GET /api/v1/messages?room_id=...&limit=...&before=...
http::HttpResponse MessageService::getMessages(const http::HttpRequest &request)
{
    //确认用户已经登录
    auto user_id_opt = getUserIdFromRequest(request);
    if(!user_id_opt)
    {
        LOG_ERROR << "User is not authenticated.";
        json error_response = {
            {"success", false},
            {"message", "Authentication required"},
            {"error", "User is not authenticated"}
        };
        return http::HttpResponse::Unauthorized().withJsonBody(error_response);
    }
    std::string user_id = *user_id_opt;

    //获取查询参数
    auto query_params = request.getQueryParams();
    auto room_id_it = query_params.find("room_id");
    if (room_id_it == query_params.end())
    {
        LOG_ERROR << "Missing 'room_id' query parameter.";
        json error_response = {
            {"success", false},
            {"message", "Missing required parameter"},
            {"error", "Missing 'room_id' query parameter"}
        };
        return http::HttpResponse::BadRequest().withJsonBody(error_response);
    }
    std::string room_id = room_id_it->second;
    
    // 检查房间是否存在
    if (!db_manager_.roomExists(room_id))
    {
        LOG_ERROR << "Room with ID '" << room_id << "' does not exist.";
        json error_response = {
            {"success", false},
            {"message", "Room not found"},
            {"error", "Room with ID '" + room_id + "' does not exist"}
        };
        return http::HttpResponse::NotFound().withJsonBody(error_response);
    }
    int limit = 50; // 默认值
    if(auto limit_opt=request.getQueryParam("limit"))
    {
        try
        {
            std::string limit_str(limit_opt->data(), limit_opt->size());
            limit = std::stoi(limit_str);
            if(limit <= 0 || limit > 100) // 限制在1到100之间
            {
                LOG_WARN << "Invalid limit value: " << limit << ". Using default value of 50.";
                limit = 50;
            }
        }
        catch(const std::exception& e)
        {
            LOG_ERROR << "Invalid limit parameter: " << std::string(limit_opt->data(), limit_opt->size()) << ". Using default value of 50.";
            limit = 50;
        }
    }

    //确认用户是该房间的成员
    auto menbers = db_manager_.getRoomMembers(room_id);
    if(std::find_if(menbers.begin(), menbers.end(), [&](const json& member) {
        return member["id"] == user_id;  // 注意：这里应该是 "id" 而不是 "user_id"
    }) == menbers.end())
    {
        LOG_ERROR << "User " << user_id << " is not a member of room " << room_id;
        json error_response = {
            {"success", false},
            {"message", "Access denied"},
            {"error", "You are not a member of this room"}
        };
        return http::HttpResponse::Forbidden().withJsonBody(error_response);
    }
    //从数据库获取消息
    try
    {
        auto messages = db_manager_.getMessages(room_id, limit);
        
        // 将 Message 对象转换为 JSON
        json message_json_array = json::array();
        std::transform(
            messages.begin(),
            messages.end(),
            std::back_inserter(message_json_array),
            [](const auto &message) {
                return message.toJson();
            }
        );
        
        json response_data = {
            {"success", true},
            {"message", "Messages retrieved successfully"},
            {"data", {
                {"messages", message_json_array},
                {"room_id", room_id},
                {"count", messages.size()}
            }}
        };
        return http::HttpResponse::Ok().withJsonBody(response_data);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << "Failed to retrieve messages for room " << room_id << ": " << e.what();
        json error_response = {
            {"success", false},
            {"message", "Failed to retrieve messages"},
            {"error", e.what()}
        };
        return http::HttpResponse::InternalError().withJsonBody(error_response);
    }

}