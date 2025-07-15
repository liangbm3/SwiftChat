#pragma once
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include <functional>

namespace middleware
{
    //认证中间件函数
    http::HttpResponse auth(
        const http::HttpRequest &request,
        const std::function<http::HttpResponse(const http::HttpRequest &)> &next
    );
}