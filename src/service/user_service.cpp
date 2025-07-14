#include "user_service.hpp"

#include "db/database_manager.hpp"
#include "http/http_server.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "utils/logger.hpp"
#include <nlohmann/json.hpp>


using json = nlohmann::json;

// 一个简单的哈希函数
static std::string hashPassword(const std::string &password)
{
    // 这里可以使用更复杂的哈希算法，如bcrypt或argon2
    return "hashed_" + password;
}

UserService::UserService(DatabaseManager &db_manager) : db_manager_(db_manager) {}

void UserService::registerRoutes(http::HttpServer &server)
{
    // 注册用户相关的HTTP路由
    server.addHandler("/api/users/register", "POST", [this](const http::HttpRequest &request) -> http::HttpResponse
                      { return handleRegister(request); });

    server.addHandler("/api/users/login", "POST", [this](const http::HttpRequest &request) -> http::HttpResponse
                      { return handleLogin(request); });
}


http::HttpResponse UserService::handleRegister(const http::HttpRequest &request)
{
    LOG_INFO << "Handling user registration request...";
    try
    {
        if(request.body.empty())
        {
            LOG_ERROR << "Request body is empty";
            return http::HttpResponse(400, "{\"error\":\"Request body is empty\"}");
        }
        json request_json = json::parse(request.body);
        std::string username = request_json.at("username").get<std::string>();
        std::string password = request_json.at("password").get<std::string>();
        if(username.empty() || password.empty())
        {
            LOG_ERROR << "Username and password cannot be empty";
            return http::HttpResponse(400, "{\"error\":\"Username and password cannot be empty\"}");
        }
        if(db_manager_.userExists(username))
        {
            LOG_ERROR << "User already exists: " << username;
            return http::HttpResponse(400, "{\"error\":\"User already exists\"}");
        }
        //使用哈希函数处理密码
        std::string password_hash = hashPassword(password);
        if(db_manager_.createUser(username, password_hash))
        {
            LOG_INFO << "User registered successfully: " << username;
            return http::HttpResponse(201, "{\"status\":\"success\",\"message\":\"User registered successfully\"}");
        }
        else
        {
            LOG_INFO << "Failed to create user: " << username;
            return http::HttpResponse(500, "{\"error\":\"Failed to create user\"}");
        }
    }
    catch(const json::exception &e)
    {
        LOG_ERROR << "JSON parsing error: " << e.what();
        return http::HttpResponse(400, "{\"error\":\"Invalid JSON format\"}");
    }
}

http::HttpResponse UserService::handleLogin(const http::HttpRequest &request)
{
    LOG_INFO << "Handling user login request...";
    try
    {
        if(request.body.empty())
        {
            LOG_ERROR << "Request body is empty";
            return http::HttpResponse(400, "{\"error\":\"Request body is empty\"}");
        }
        json request_json = json::parse(request.body);
        std::string username = request_json.at("username").get<std::string>();
        std::string password = request_json.at("password").get<std::string>();
        if(username.empty() || password.empty())
        {
            LOG_ERROR << "Username and password cannot be empty";
            return http::HttpResponse(400, "{\"error\":\"Username and password cannot be empty\"}");
        }
        std::string password_hash = hashPassword(password);
        if(db_manager_.validateUser(username, password_hash))
        {
            db_manager_.setUserOnlineStatus(username, true);
            db_manager_.updateUserLastActiveTime(username);
            // 在实际应用中，登录成功后应返回一个session token或JWT，用于后续请求的身份验证。
            LOG_INFO << "User logged in successfully: " << username;
            return http::HttpResponse(200, "{\"status\":\"success\",\"message\":\"User logged in successfully\"}");
        }
        else
        {
            LOG_ERROR << "Invalid username or password for user: " << username;
            return http::HttpResponse(401, "{\"error\":\"Invalid username or password\"}");// 401 Unauthorized
        }
    }
    catch(const json::exception &e)
    {
        LOG_ERROR << "JSON parsing error: " << e.what();
        return http::HttpResponse(400, "{\"error\":\"Invalid JSON format\"}");
    }
}