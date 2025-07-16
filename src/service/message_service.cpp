#include "message_service.hpp"
#include "db/database_manager.hpp"
#include <jwt-cpp/jwt.h>
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
    auto auth_header = request.getHeaderValue("Authorization");
    if(!auth_header)
    {
        LOG_ERROR << "Authorization header is missing in the request.";
        return std::nullopt;
    }

    std::string token_str = std::string(*auth_header);
    const std::string bearer_prefix = "Bearer ";
    if (token_str.rfind(bearer_prefix, 0) != 0) {
        LOG_ERROR << "Invalid token format. Expected 'Bearer <token>'.";
        return std::nullopt;
    }

    // 去掉 "Bearer " 前缀
    token_str.erase(0, bearer_prefix.length());
    
    try
    {
        // 获取与签发时相同的密钥
        const char *secret_key_cstr = std::getenv("JWT_SECRET");
        if (!secret_key_cstr) {
            LOG_ERROR << "JWT_SECRET_KEY environment variable not set";
            return std::nullopt;
        }
        std::string secret_key(secret_key_cstr);

        // 解码和验证 JWT 令牌
        auto decoded_token = jwt::decode(token_str);
        auto verifier = jwt::verify()
                            .allow_algorithm(jwt::algorithm::hs256{secret_key})
                            .with_issuer("SwiftChat");

        verifier.verify(decoded_token);
        
        // 验证通过，返回用户ID
        return decoded_token.get_subject(); // 使用 subject 声明获取用户ID
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << "Failed to decode or verify JWT: " << e.what();
        return std::nullopt;
    }
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
        json response_data = {
            {"success", true},
            {"message", "Messages retrieved successfully"},
            {"data", {
                {"messages", messages},
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