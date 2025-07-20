#include "jwt_utils.hpp"
#include "http/http_request.hpp"
#include "utils/logger.hpp"
#include <jwt-cpp/jwt.h>
#include <cstdlib>

const std::string JwtUtils::BEARER_PREFIX = "Bearer ";
const std::string JwtUtils::JWT_ISSUER = "SwiftChat";

std::optional<std::string> JwtUtils::getUserIdFromRequest(const http::HttpRequest& request)
{
    auto auth_header = request.getHeaderValue("Authorization");
    if (!auth_header)
    {
        LOG_ERROR << "Authorization header is missing in the request.";
        return std::nullopt;
    }

    // 将string_view转换为string
    std::string auth_header_str(*auth_header);
    auto token = extractBearerToken(auth_header_str);
    if (!token)
    {
        return std::nullopt;
    }

    return verifyToken(*token);
}

std::optional<std::string> JwtUtils::verifyToken(const std::string& token)
{
    try
    {
        // 获取与签发时相同的密钥
        const char* secret_key_cstr = std::getenv("JWT_SECRET");
        if (!secret_key_cstr)
        {
            LOG_ERROR << "JWT_SECRET environment variable not set";
            return std::nullopt;
        }
        std::string secret_key(secret_key_cstr);

        // 解码和验证 JWT 令牌
        auto decoded_token = jwt::decode(token);
        auto verifier = jwt::verify()
                            .allow_algorithm(jwt::algorithm::hs256{secret_key})
                            .with_issuer(JWT_ISSUER);

        verifier.verify(decoded_token);
        
        // 验证通过，返回用户ID
        return decoded_token.get_subject(); // 使用 subject 声明获取用户ID
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Failed to decode or verify JWT: " << e.what();
        return std::nullopt;
    }
}

std::optional<std::string> JwtUtils::extractBearerToken(const std::string& auth_header)
{
    if (auth_header.rfind(BEARER_PREFIX, 0) != 0)
    {
        LOG_ERROR << "Invalid token format. Expected 'Bearer <token>'.";
        return std::nullopt;
    }

    // 去掉 "Bearer " 前缀
    std::string token = auth_header.substr(BEARER_PREFIX.length());
    
    // 检查令牌是否为空
    if (token.empty())
    {
        LOG_ERROR << "Empty token after Bearer prefix.";
        return std::nullopt;
    }

    return token;
}
