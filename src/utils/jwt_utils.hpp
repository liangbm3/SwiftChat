#pragma once

#include <string>
#include <optional>

// 前向声明
namespace http {
    class HttpRequest;
}

/**
 * JWT工具类
 * 提供JWT令牌的验证和用户ID提取功能
 */
class JwtUtils
{
public:
    /**
     * 从HTTP请求中提取并验证JWT令牌，返回用户ID
     * @param request HTTP请求对象
     * @return 如果验证成功返回用户ID，否则返回空
     */
    static std::optional<std::string> getUserIdFromRequest(const http::HttpRequest& request);

    /**
     * 验证JWT令牌
     * @param token JWT令牌字符串
     * @return 如果验证成功返回用户ID，否则返回空
     */
    static std::optional<std::string> verifyToken(const std::string& token);

    /**
     * 从Authorization头部提取Bearer令牌
     * @param auth_header Authorization头部的值
     * @return 如果格式正确返回令牌，否则返回空
     */
    static std::optional<std::string> extractBearerToken(const std::string& auth_header);

private:
    static const std::string BEARER_PREFIX;
    static const std::string JWT_ISSUER;
};
