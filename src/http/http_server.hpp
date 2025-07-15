#pragma once

#include <string>
#include <functional>
#include <vector>
#include <unordered_map>
#include "utils/thread_pool.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"

namespace http
{
    class HttpServer
    {
    public:
        // 请求处理函数，接收Request，返回Response
        using RequestHandler = std::function<HttpResponse(const HttpRequest &)>;

        // 中间件函数，可以对请求和响应进行预处理和后处理
        // 它接收一个请求和一个“下一个”处理函数，并返回一个响应
        using Middleware = std::function<HttpResponse(const HttpRequest &, const RequestHandler &)>;

        explicit HttpServer(int port, size_t thread_count = std::thread::hardware_concurrency());
        ~HttpServer();

        // 注册API路由处理函数
        void addHandler(const std::string &path, const std::string &method, RequestHandler handler);

        // 注册中间件
        void addMiddleware(Middleware middleware);

        // 设置静态文件目录
        void setStaticDirectory(const std::string &dir);

        void run();
        void stop();

        // 测试可访问的路由方法
        HttpResponse routeRequest(const HttpRequest &request); // 路由分发逻辑
        HttpResponse serveStaticFile(const std::string &path); // 返回HttpResponse对象

    private:
        int port_;
        int server_fd_;
        bool running_;
        utils::ThreadPool thread_pool_;
        std::string static_dir_;

        // 路由表：Path -> (Method -> Handler)
        std::unordered_map<std::string, std::unordered_map<std::string, RequestHandler>> handlers_;
        // 中间件链
        std::vector<Middleware> middleware_chain_;

        // MIME类型映射表，设为静态常量以提高效率
        static const std::unordered_map<std::string, std::string> MIME_TYPES;

        void handleClient(int client_fd);
    };
}