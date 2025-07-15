#include "middleware/auth_middleware.hpp"
#include <jwt-cpp/jwt.h>
#include <cstdlib>
#include "utils/logger.hpp"

namespace middleware
{

    http::HttpResponse auth(
        const http::HttpRequest &req,
        const std::function<http::HttpResponse(const http::HttpRequest &)> &next)
    {
        //从请求头中获取 Authorization字段
        auto auth_header = req.getHeaderValue("Authorization");
        if (!auth_header)
        {
            LOG_ERROR << "Authorization header is missing in the request.";
            return http::HttpResponse::Unauthorized("Missing Authorization Header");
        }

        //检查格式是否为 "Bearer <token>"
        std::string token_str = std::string(*auth_header);
        const std::string bearer_prefix = "Bearer ";
        if (token_str.rfind(bearer_prefix, 0) != 0)
        {
            LOG_ERROR << "Invalid token format. Expected 'Bearer <token>'.";
            return http::HttpResponse::Unauthorized("Invalid token format. Expected 'Bearer <token>'.");
        }

        //去掉 "Bearer " 前缀
        token_str.erase(0, bearer_prefix.length());

        try
        {
            //获取与签发时相同的密钥
            const char *secret_key_cstr = std::getenv("JWT_SECRET_KEY");
            if (!secret_key_cstr)
            {
                LOG_ERROR << "FATAL: JWT_SECRET_KEY not set for verification.";
                return http::HttpResponse::InternalError("Server configuration error.");
            }
            std::string secret_key(secret_key_cstr);

            //解码和验证 JWT 令牌
            auto decoded_token = jwt::decode(token_str);
            auto verifier = jwt::verify()
                                .allow_algorithm(jwt::algorithm::hs256{secret_key})
                                .with_issuer("SwiftChat");

            verifier.verify(decoded_token);

            // 验证通过，调用下一个处理器
            return next(req);
        }
        catch (const jwt::error::token_verification_exception &e)
        {
            LOG_ERROR << "JWT verification failed: " << e.what();
            return http::HttpResponse::Unauthorized("Invalid token: " + std::string(e.what()));
        }
        catch (const jwt::error::signature_verification_exception &e)
        {
            LOG_ERROR << "JWT signature verification failed: " << e.what();
            return http::HttpResponse::Unauthorized("Invalid token signature: " + std::string(e.what()));
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "Unexpected error: " << e.what();
            return http::HttpResponse::BadRequest("Invalid token.");
        }
    }

} // namespace middleware