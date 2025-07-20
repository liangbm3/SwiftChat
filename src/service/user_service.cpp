#include "user_service.hpp"
#include "service/user_status_manager.hpp"
#include "db/database_manager.hpp"
#include "http/http_server.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "utils/logger.hpp"
#include "utils/jwt_utils.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <optional>
#include <regex>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;

UserService::UserService(DatabaseManager &db_manager) : db_manager_(db_manager) {}

void UserService::setStatusManager(std::shared_ptr<UserStatusManager> status_manager)
{
    status_manager_ = status_manager;
}

void UserService::registerRoutes(http::HttpServer &server)
{
    // 注册获取当前用户信息的路由
    server.addHandler({
        .path = "/api/v1/users/me",
        .method = "GET",
        .handler = [this](const http::HttpRequest &request) { return handleGetCurrentUser(request); },
        .use_auth_middleware = true
    });

    // 注册获取在线用户列表的路由 (必须在 users/{userId} 之前)
    server.addHandler({
        .path = "/api/v1/users/online",
        .method = "GET",
        .handler = [this](const http::HttpRequest &request) { return handleGetOnlineUsers(request); },
        .use_auth_middleware = true
    });

    // 注册获取所有用户列表的路由
    server.addHandler({
        .path = "/api/v1/users",
        .method = "GET",
        .handler = [this](const http::HttpRequest &request) { return handleGetAllUsers(request); },
        .use_auth_middleware = true
    });

    // 注册获取指定用户信息的路由
    server.addHandler({
        .path = "/api/v1/users/{userId}",
        .method = "GET",
        .handler = [this](const http::HttpRequest &request) { return handleGetUserById(request); },
        .use_auth_middleware = true
    });

    // 注册获取用户状态的路由
    server.addHandler({
        .path = "/api/v1/users/{userId}/status",
        .method = "GET",
        .handler = [this](const http::HttpRequest &request) { return handleGetUserStatus(request); },
        .use_auth_middleware = true
    });
}

std::optional<std::string> UserService::getUserIdFromRequest(const http::HttpRequest &request)
{
    return JwtUtils::getUserIdFromRequest(request);
}

http::HttpResponse UserService::handleGetCurrentUser(const http::HttpRequest &request)
{
    auto user_id_opt = getUserIdFromRequest(request);
    if (!user_id_opt)
    {
        LOG_ERROR << "Failed to get user ID from request.";
        json error_response = {
            {"success", false},
            {"message", "Authentication required"},
            {"error", "User is not authenticated"}
        };
        return http::HttpResponse::Unauthorized().withJsonBody(error_response);
    }
    std::string user_id = *user_id_opt;

    try
    {
        // 从数据库获取用户信息
        auto user_opt = db_manager_.getUserById(user_id);
        if (!user_opt)
        {
            LOG_ERROR << "User with ID '" << user_id << "' not found in database.";
            json error_response = {
                {"success", false},
                {"message", "User not found"},
                {"error", "User with ID '" + user_id + "' does not exist"}
            };
            return http::HttpResponse::NotFound().withJsonBody(error_response);
        }

        // 将用户对象转换为JSON，不包含敏感信息
        json user_json = user_opt->toJson();
        // 移除密码字段（如果存在）
        user_json.erase("password");
        user_json.erase("password_hash");

        json response_data = {
            {"success", true},
            {"message", "Current user information retrieved successfully"},
            {"data", {
                {"user", user_json}
            }}
        };
        return http::HttpResponse::Ok().withJsonBody(response_data);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Failed to retrieve current user information: " << e.what();
        json error_response = {
            {"success", false},
            {"message", "Failed to retrieve user information"},
            {"error", e.what()}
        };
        return http::HttpResponse::InternalError().withJsonBody(error_response);
    }
}

http::HttpResponse UserService::handleGetAllUsers(const http::HttpRequest &request)
{
    auto user_id_opt = getUserIdFromRequest(request);
    if (!user_id_opt)
    {
        LOG_ERROR << "Failed to get user ID from request.";
        json error_response = {
            {"success", false},
            {"message", "Authentication required"},
            {"error", "User is not authenticated"}
        };
        return http::HttpResponse::Unauthorized().withJsonBody(error_response);
    }
    std::string current_user_id = *user_id_opt;

    try
    {
        // 获取查询参数
        int limit = 50; // 默认限制
        int offset = 0; // 默认偏移量

        if (auto limit_opt = request.getQueryParam("limit"))
        {
            try
            {
                std::string limit_str(limit_opt->data(), limit_opt->size());
                limit = std::stoi(limit_str);
                if (limit <= 0 || limit > 100) // 限制在1到100之间
                {
                    LOG_WARN << "Invalid limit value: " << limit << ". Using default value of 50.";
                    limit = 50;
                }
            }
            catch (const std::exception& e)
            {
                LOG_ERROR << "Invalid limit parameter. Using default value of 50.";
                limit = 50;
            }
        }

        if (auto offset_opt = request.getQueryParam("offset"))
        {
            try
            {
                std::string offset_str(offset_opt->data(), offset_opt->size());
                offset = std::stoi(offset_str);
                if (offset < 0)
                {
                    LOG_WARN << "Invalid offset value: " << offset << ". Using default value of 0.";
                    offset = 0;
                }
            }
            catch (const std::exception& e)
            {
                LOG_ERROR << "Invalid offset parameter. Using default value of 0.";
                offset = 0;
            }
        }

        // 从数据库获取用户列表
        auto all_users = db_manager_.getAllUsers();
        
        // 在服务层实现分页逻辑
        size_t total_count = all_users.size();
        size_t start_index = std::min(static_cast<size_t>(offset), total_count);
        size_t end_index = std::min(start_index + static_cast<size_t>(limit), total_count);
        
        std::vector<User> users;
        if (start_index < total_count) {
            users.assign(all_users.begin() + start_index, all_users.begin() + end_index);
        }
        
        // 将用户对象转换为JSON数组，不包含敏感信息
        json users_json_array = json::array();
        for (const auto& user : users)
        {
            json user_json = user.toJson();
            // 移除密码字段（如果存在）
            user_json.erase("password");
            user_json.erase("password_hash");
            users_json_array.push_back(user_json);
        }

        json response_data = {
            {"success", true},
            {"message", "Users list retrieved successfully"},
            {"data", {
                {"users", users_json_array},
                {"count", users.size()},
                {"total", total_count},
                {"limit", limit},
                {"offset", offset}
            }}
        };
        return http::HttpResponse::Ok().withJsonBody(response_data);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Failed to retrieve users list: " << e.what();
        json error_response = {
            {"success", false},
            {"message", "Failed to retrieve users list"},
            {"error", e.what()}
        };
        return http::HttpResponse::InternalError().withJsonBody(error_response);
    }
}

http::HttpResponse UserService::handleGetUserById(const http::HttpRequest &request)
{
    auto current_user_id_opt = getUserIdFromRequest(request);
    if (!current_user_id_opt)
    {
        LOG_ERROR << "Failed to get user ID from request.";
        json error_response = {
            {"success", false},
            {"message", "Authentication required"},
            {"error", "User is not authenticated"}
        };
        return http::HttpResponse::Unauthorized().withJsonBody(error_response);
    }
    std::string current_user_id = *current_user_id_opt;

    // 从URL路径中提取用户ID
    std::string path = request.getPath();
    std::regex user_id_regex(R"(/api/v1/users/([^/]+))");
    std::smatch matches;
    
    if (!std::regex_match(path, matches, user_id_regex) || matches.size() != 2)
    {
        LOG_ERROR << "Invalid URL format for user ID extraction: " << path;
        json error_response = {
            {"success", false},
            {"message", "Invalid request format"},
            {"error", "Invalid URL format"}
        };
        return http::HttpResponse::BadRequest().withJsonBody(error_response);
    }
    
    std::string target_user_id = matches[1].str();

    try
    {
        // 从数据库获取指定用户信息
        auto user_opt = db_manager_.getUserById(target_user_id);
        if (!user_opt)
        {
            LOG_ERROR << "User with ID '" << target_user_id << "' not found in database.";
            json error_response = {
                {"success", false},
                {"message", "User not found"},
                {"error", "User with ID '" + target_user_id + "' does not exist"}
            };
            return http::HttpResponse::NotFound().withJsonBody(error_response);
        }

        // 将用户对象转换为JSON，不包含敏感信息
        json user_json = user_opt->toJson();
        // 移除密码字段（如果存在）
        user_json.erase("password");
        user_json.erase("password_hash");

        json response_data = {
            {"success", true},
            {"message", "User information retrieved successfully"},
            {"data", {
                {"user", user_json}
            }}
        };
        return http::HttpResponse::Ok().withJsonBody(response_data);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Failed to retrieve user information for ID '" << target_user_id << "': " << e.what();
        json error_response = {
            {"success", false},
            {"message", "Failed to retrieve user information"},
            {"error", e.what()}
        };
        return http::HttpResponse::InternalError().withJsonBody(error_response);
    }
}

http::HttpResponse UserService::handleGetOnlineUsers(const http::HttpRequest &request)
{
    auto user_id_opt = getUserIdFromRequest(request);
    if (!user_id_opt)
    {
        LOG_ERROR << "Failed to get user ID from request.";
        json error_response = {
            {"success", false},
            {"message", "Authentication required"},
            {"error", "User is not authenticated"}
        };
        return http::HttpResponse::Unauthorized().withJsonBody(error_response);
    }

    try
    {
        if (!status_manager_)
        {
            LOG_WARN << "UserStatusManager not available";
            json error_response = {
                {"success", false},
                {"message", "User status service unavailable"},
                {"error", "Status manager not initialized"}
            };
            return http::HttpResponse::InternalError().withJsonBody(error_response);
        }

        auto online_user_ids = status_manager_->getOnlineUsers();
        auto stats = status_manager_->getOnlineStats();

        // 获取在线用户的详细信息
        json online_users_array = json::array();
        for (const std::string& online_user_id : online_user_ids)
        {
            auto user_opt = db_manager_.getUserById(online_user_id);
            if (user_opt)
            {
                auto user_json = user_opt->toJson();
                user_json["is_online"] = true;
                
                // 添加在线状态信息
                auto session = status_manager_->getUserSession(online_user_id);
                if (session)
                {
                    user_json["connection_type"] = session->connection_type;
                    auto duration = status_manager_->getOnlineDuration(online_user_id);
                    user_json["online_duration_seconds"] = duration.count();
                }
                
                online_users_array.push_back(user_json);
            }
        }

        json response_data = {
            {"success", true},
            {"message", "Online users retrieved successfully"},
            {"data", {
                {"users", online_users_array},
                {"stats", {
                    {"total_online", stats.total_online},
                    {"websocket_connections", stats.websocket_connections},
                    {"http_sessions", stats.http_sessions}
                }}
            }}
        };
        return http::HttpResponse::Ok().withJsonBody(response_data);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Failed to retrieve online users: " << e.what();
        json error_response = {
            {"success", false},
            {"message", "Failed to retrieve online users"},
            {"error", e.what()}
        };
        return http::HttpResponse::InternalError().withJsonBody(error_response);
    }
}

http::HttpResponse UserService::handleGetUserStatus(const http::HttpRequest &request)
{
    auto user_id_opt = getUserIdFromRequest(request);
    if (!user_id_opt)
    {
        LOG_ERROR << "Failed to get user ID from request.";
        json error_response = {
            {"success", false},
            {"message", "Authentication required"},
            {"error", "User is not authenticated"}
        };
        return http::HttpResponse::Unauthorized().withJsonBody(error_response);
    }

    // 获取路径参数中的目标用户ID
    auto target_user_id_opt = request.getPathParam("userId");
    if (!target_user_id_opt)
    {
        LOG_ERROR << "Missing userId path parameter.";
        json error_response = {
            {"success", false},
            {"message", "Missing required parameter"},
            {"error", "Missing 'userId' path parameter"}
        };
        return http::HttpResponse::BadRequest().withJsonBody(error_response);
    }

    std::string target_user_id = std::string(*target_user_id_opt);

    try
    {
        // 检查目标用户是否存在
        auto user_opt = db_manager_.getUserById(target_user_id);
        if (!user_opt)
        {
            LOG_ERROR << "User with ID '" << target_user_id << "' does not exist.";
            json error_response = {
                {"success", false},
                {"message", "User not found"},
                {"error", "User with ID '" + target_user_id + "' does not exist"}
            };
            return http::HttpResponse::NotFound().withJsonBody(error_response);
        }

        json status_data = {
            {"user_id", target_user_id},
            {"username", user_opt->getUsername()},
            {"is_online", false},
            {"connection_type", ""},
            {"online_duration_seconds", 0},
            {"last_activity", ""}
        };

        if (status_manager_)
        {
            bool is_online = status_manager_->isUserOnline(target_user_id);
            status_data["is_online"] = is_online;

            if (is_online)
            {
                auto session = status_manager_->getUserSession(target_user_id);
                if (session)
                {
                    status_data["connection_type"] = session->connection_type;
                    auto duration = status_manager_->getOnlineDuration(target_user_id);
                    status_data["online_duration_seconds"] = duration.count();
                    
                    // 转换时间为ISO 8601字符串格式
                    auto last_activity = status_manager_->getLastActivity(target_user_id);
                    auto time_t = std::chrono::system_clock::to_time_t(last_activity);
                    std::ostringstream oss;
                    oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
                    status_data["last_activity"] = oss.str();
                }
            }
        }

        json response_data = {
            {"success", true},
            {"message", "User status retrieved successfully"},
            {"data", {
                {"status", status_data}
            }}
        };
        return http::HttpResponse::Ok().withJsonBody(response_data);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Failed to retrieve user status for ID '" << target_user_id << "': " << e.what();
        json error_response = {
            {"success", false},
            {"message", "Failed to retrieve user status"},
            {"error", e.what()}
        };
        return http::HttpResponse::InternalError().withJsonBody(error_response);
    }
}
