#include "middleware/auth_middleware.hpp"
#include "utils/jwt_utils.hpp"
#include <cstdlib>
#include "utils/logger.hpp"

namespace middleware
{

    http::HttpResponse auth(
        const http::HttpRequest &req,
        const std::function<http::HttpResponse(const http::HttpRequest &)> &next)
    {
        // 使用JWT工具类验证令牌
        auto user_id = JwtUtils::getUserIdFromRequest(req);
        if (!user_id)
        {
            LOG_ERROR << "JWT token verification failed";
            return http::HttpResponse::Unauthorized("Invalid or missing authentication token");
        }

        LOG_INFO << "JWT token verified successfully for user ID: " << *user_id;
        
        // 验证通过，调用下一个处理器
        return next(req);
    }

} // namespace middleware