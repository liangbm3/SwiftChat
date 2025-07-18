#include "service/auth_service.hpp"
#include "db/database_manager.hpp"
#include "http/http_server.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include <nlohmann/json.hpp>
#include <jwt-cpp/jwt.h>
#include <chrono>
#include <cstdlib>          // 用于 getenv
#include "utils/logger.hpp" // 日志记录工具

using json = nlohmann::json;

AuthService::AuthService(DatabaseManager &db_manager) : db_manager_(db_manager) {}

void AuthService::registerRoutes(http::HttpServer &server)
{
    http::HttpServer::Route register_route{
        .path = "/api/v1/auth/register",
        .method = "POST",
        .handler = [this](const http::HttpRequest &request) -> http::HttpResponse {
            return registerUser(request);
        },
        .use_auth_middleware = false // 注册不需要认证
    };
    server.addHandler(register_route);

    http::HttpServer::Route login_route{
        .path = "/api/v1/auth/login",
        .method = "POST",
        .handler = [this](const http::HttpRequest &request) -> http::HttpResponse {
            return loginUser(request);
        },
        .use_auth_middleware = false // 登录不需要认证
    };
    server.addHandler(login_route);
}



http::HttpResponse AuthService::registerUser(const http::HttpRequest &request)
{
    try
    {
        LOG_INFO << "Processing user registration request";
        //解析请求体
        json request_body = json::parse(request.getBody());
        std::string username = request_body.at("username").get<std::string>();
        std::string password = request_body.at("password").get<std::string>();
        
        LOG_INFO << "Registration request for username: " << username;

        //检查用户是否存在
        if (db_manager_.userExists(username))
        {
            LOG_WARN << "User already exists: " << username;
            json error_response = {
                {"success", false},
                {"message", "User already exists"},
                {"error", "Username is already taken"}
            };
            return http::HttpResponse::BadRequest().withJsonBody(error_response);
        }
        
        LOG_INFO << "User does not exist, proceeding with registration for: " << username;

        //对密码进行哈希处理
        std::string password_hash = hashPassword(password);
        LOG_INFO << "Password hashed for user: " << username;
        
        //创建用户
        LOG_INFO << "Attempting to create user in database: " << username;
        if (!db_manager_.createUser(username, password_hash))
        {
            LOG_ERROR << "Failed to create user: " << username;
            json error_response = {
                {"success", false},
                {"message", "Failed to create user"},
                {"error", "Database operation failed"}
            };
            return http::HttpResponse::InternalError().withJsonBody(error_response);
        }
        
        LOG_INFO << "User created successfully in database: " << username;
        
        //通过用户名获取完整的信息
        auto user = db_manager_.getUserByUsername(username);
        if (!user)
        {
            LOG_ERROR << "Failed to retrieve user after creation: " << username;
            json error_response = {
                {"success", false},
                {"message", "Failed to retrieve user after creation"},
                {"error", "Database operation failed"}
            };
            return http::HttpResponse::InternalError().withJsonBody(error_response);
        }

        // 生成 JWT 令牌
        return createAndSignToken(*user);
    }
    catch(const json::exception &e)
    {
        LOG_ERROR << "JSON parsing error: " << e.what();
        json error_response = {
            {"success", false},
            {"message", "Invalid JSON format"},
            {"error", e.what()}
        };
        return http::HttpResponse::BadRequest().withJsonBody(error_response);
    }
    catch(const std::exception &e)
    {
        LOG_ERROR << "Exception during user registration: " << e.what();
        json error_response = {
            {"success", false},
            {"message", "Internal server error"},
            {"error", e.what()}
        };
        return http::HttpResponse::InternalError().withJsonBody(error_response);
    }
    catch(...)
    {
        LOG_ERROR << "Unknown exception during user registration";
        json error_response = {
            {"success", false},
            {"message", "Unknown error occurred"},
            {"error", "Unknown exception"}
        };
        return http::HttpResponse::InternalError().withJsonBody(error_response);
    }
    
}


http::HttpResponse AuthService::loginUser(const http::HttpRequest &request)
{
    try
    {
        //解析请求体
        json request_body = json::parse(request.getBody());
        std::string username = request_body.at("username").get<std::string>();
        std::string password = request_body.at("password").get<std::string>();

        //验证用户
        if (!db_manager_.validateUser(username, hashPassword(password)))
        {
            LOG_WARN << "Invalid login attempt for user: " << username;
            json error_response = {
                {"success", false},
                {"message", "Invalid username or password"},
                {"error", "Authentication failed"}
            };
            return http::HttpResponse::Unauthorized().withJsonBody(error_response);
        }

        //获取用户信息
        auto user = db_manager_.getUserByUsername(username);
        if (!user)
        {
            LOG_ERROR << "Failed to retrieve user during login: " << username;
            json error_response = {
                {"success", false},
                {"message", "Failed to retrieve user"},
                {"error", "Database operation failed"}
            };
            return http::HttpResponse::InternalError().withJsonBody(error_response);
        }

        //更新用户最后活跃时间
        db_manager_.updateUserLastActiveTime(user->getId());

        // 生成 JWT 令牌
        return createAndSignToken(*user);
    }
    catch(const json::exception &e)
    {
        LOG_ERROR << "JSON parsing error: " << e.what();
        json error_response = {
            {"success", false},
            {"message", "Invalid JSON format"},
            {"error", e.what()}
        };
        return http::HttpResponse::BadRequest().withJsonBody(error_response);
    }
    catch(const std::exception &e)
    {
        LOG_ERROR << "Exception during user login: " << e.what();
        json error_response = {
            {"success", false},
            {"message", "Internal server error"},
            {"error", e.what()}
        };
        return http::HttpResponse::InternalError().withJsonBody(error_response);
    
    }
    catch(...)
    {
        LOG_ERROR << "Unknown exception during user login";
        json error_response = {
            {"success", false},
            {"message", "Unknown error occurred"},
            {"error", "Unknown exception"}
        };
        return http::HttpResponse::InternalError().withJsonBody(error_response);
    }
    
}

http::HttpResponse AuthService::createAndSignToken(const User &user)
{
    //从环境变量中读取密钥
    const char *secret = std::getenv("JWT_SECRET");
    if(!secret)
    {
        LOG_ERROR << "JWT_SECRET environment variable not set";
        json error_response = {
            {"success", false},
            {"message", "Server configuration error"},
            {"error", "JWT secret not configured"}
        };
        return http::HttpResponse::InternalError().withJsonBody(error_response);
    }
    std::string secret_key(secret);// 将 C 风格字符串转换为 std::string
    //创建JWT令牌
    auto token = jwt::create()
        .set_issuer("SwiftChat")//签发者
        .set_type("JWT")//令牌类型
        .set_issued_at(std::chrono::system_clock::now())//签发时间
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(1))//过期时间，1小时后
        .set_subject(user.getId())//设置主题 (sub)，这是标准声明，通常用来存放用户的唯一标识符
        .set_payload_claim("username", jwt::claim(user.getUsername()))//自定义声明，存放用户名
        .set_payload_claim("last_active", jwt::claim(std::to_string(user.getLastActiveTime())))//自定义声明，存放最后活跃时间
        .sign(jwt::algorithm::hs256{secret_key});//使用哈希算法HS256和密钥进行签名
    
    //构造HTTP响应
    json response_json = {
        {"success", true},
        {"message", "Authentication successful"},
        {"data", {
            {"token", token},
            {"user_id", user.getId()},
            {"username", user.getUsername()}
        }}
    };
    return http::HttpResponse::Ok().withJsonBody(response_json); // 返回200 OK响应，包含JSON格式的令牌和消息
}


std::string AuthService::hashPassword(const std::string &password)
{
    // 使用 bcrypt 或其他哈希算法对密码进行哈希
    // 这里可以使用第三方库如 bcryptcpp 或 OpenSSL
    return password + "_hashed";
}