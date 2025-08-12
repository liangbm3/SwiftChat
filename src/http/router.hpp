#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include "http_request.hpp"
#include "http_response.hpp"

namespace http
{
    class Router
    {
    public:
        // 请求处理函数，接收Request，返回Response
        using Handler = std::function<HttpResponse(const HttpRequest &)>;
        // 中间件函数，接收Request和下一个RequestHandler，并返回一个响应
        using Middleware = std::function<HttpResponse(const HttpRequest &, const Handler &)>;
        struct Route
        {
            std::string path;         // 路由路径
            std::string method;       // HTTP方法，如 GET、POST 等
            Handler handler;          // 处理函数
            bool use_auth_middleware; // 是否使用认证中间件
        };

        Router();

        // 配置接口
        void addHandler(const Route &route);             // 注册API路由处理函数
        void setMiddleware(Middleware middleware);       // 注册中间件
        void setStaticDirectory(const std::string &dir); // 设置静态文件目录

        // 核心方法，接收请求，返回响应
        HttpResponse route(const HttpRequest &request);

    private:
        // 成员函数
        HttpResponse serveStaticFile(const std::string &path); // 提供静态文件服务
        bool matchPath(const std::string &pattern,
                       const std::string &path,
                       std::unordered_map<std::string, std::string> &params); // 路径参数匹配和提取

        // 成员变量
        std::vector<Route> routes_; // 路由表
        std::string static_dir_;    // 静态文件目录
        Middleware middleware_;     // 中间件
        static const std::unordered_map<std::string, std::string> MIME_TYPES; // MIME类型映射表
    };
}