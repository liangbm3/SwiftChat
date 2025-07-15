#include "http_server.hpp"
#include "utils/logger.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>
#include <sys/stat.h>

namespace http
{
    // 初始化静态MIME类型映射表
    const std::unordered_map<std::string, std::string> HttpServer::MIME_TYPES = {
        {"html", "text/html"}, {"css", "text/css"}, {"js", "application/javascript"}, {"json", "application/json"}, {"png", "image/png"}, {"jpg", "image/jpeg"}, {"jpeg", "image/jpeg"}, {"gif", "image/gif"}, {"svg", "image/svg+xml"}, {"ico", "image/x-icon"}, {"txt", "text/plain"}};

    HttpServer::HttpServer(int port, size_t thread_count)
        : port_(port), running_(false), thread_pool_(thread_count)
    {
        // (构造函数中的socket, setsockopt, bind, listen等底层代码与您原来的一样，保持不变)
        // ...
    }

    HttpServer::~HttpServer()
    {
        stop();
        if (server_fd_ >= 0)
            close(server_fd_);
    }

    void HttpServer::addHandler(const std::string &path, const std::string &method, RequestHandler handler)
    {
        handlers_[path][method] = std::move(handler);
    }

    void HttpServer::addMiddleware(Middleware middleware)
    {
        middleware_chain_.push_back(std::move(middleware));
    }

    void HttpServer::setStaticDirectory(const std::string &dir)
    {
        static_dir_ = dir;
    }

    void HttpServer::run()
    {
        // (run函数中的accept循环与您原来的一样，保持不变)
        // ...
    }

    void HttpServer::stop()
    {
        // (stop函数的实现与您原来的一样，保持不变)
        // ...
    }

    // 核心客户端处理逻辑
    void HttpServer::handleClient(int client_fd)
    {
        try
        {
            const size_t BUFFER_SIZE = 8192;
            char buffer[BUFFER_SIZE];
            std::string request_data;

            // 1. 读取请求数据 (简化版，适用于大多数情况)
            ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
            if (bytes_received <= 0)
            {
                // ... (处理错误或断开连接)
                close(client_fd);
                return;
            }
            request_data.append(buffer, bytes_received);

            // 2. [适配] 使用新的 HttpRequest API
            auto request_opt = HttpRequest::parse(request_data);
            HttpResponse response;

            if (!request_opt)
            {
                // 解析失败，返回400 Bad Request
                response = HttpResponse::BadRequest("Invalid HTTP request format.");
            }
            else
            {
                HttpRequest &request = *request_opt;
                LOG_INFO << "Request: " << request.getMethod() << " " << request.getPath();

                // 3. [优化] 应用中间件和路由
                response = routeRequest(request);
            }

            // 4. 发送响应
            std::string response_str = response.toString();
            send(client_fd, response_str.c_str(), response_str.length(), 0);
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "Exception in handleClient: " << e.what();
            // 确保即使有异常也尝试发送500错误
            auto error_response = HttpResponse::InternalError().toString();
            send(client_fd, error_response.c_str(), error_response.length(), 0);
        }
        close(client_fd);
    }

    // [新增] 路由与中间件处理
    HttpResponse HttpServer::routeRequest(const HttpRequest &request)
    {
        // 最终要执行的核心业务逻辑（API路由或静态文件服务）
        RequestHandler final_handler = [this](const HttpRequest &req) -> HttpResponse
        {
            auto path_it = handlers_.find(req.getPath());
            if (path_it != handlers_.end())
            {
                auto method_it = path_it->second.find(req.getMethod());
                if (method_it != path_it->second.end())
                {
                    return method_it->second(req); // 找到API处理器
                }
                return HttpResponse::BadRequest("Method Not Allowed");
            }

            // 检查是否为静态文件请求
            if (req.getMethod() == "GET")
            {
                return serveStaticFile(req.getPath());
            }

            return HttpResponse::NotFound("Endpoint not found");
        };

        // 将中间件从后往前串联成一个调用链
        auto chain = final_handler;
        for (auto it = middleware_chain_.rbegin(); it != middleware_chain_.rend(); ++it)
        {
            chain = [=](const HttpRequest &req)
            {
                return (*it)(req, chain);
            };
        }

        // 从调用链的顶端开始执行
        return chain(request);
    }

    // [优化] 返回HttpResponse对象，而不是修改引用
    HttpResponse HttpServer::serveStaticFile(const std::string &path)
    {
        std::string safe_path = path;
        // 基础安全检查：防止目录遍历攻击
        if (safe_path.find("..") != std::string::npos)
        {
            return HttpResponse::Forbidden("Path traversal not allowed.");
        }

        std::string full_path = static_dir_ + (path == "/" ? "/index.html" : path);

        std::ifstream file(full_path, std::ios::binary);
        if (!file)
        {
            return HttpResponse::NotFound("Static file not found.");
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();

        auto ext_pos = full_path.find_last_of('.');
        std::string mime_type = "application/octet-stream"; // 默认
        if (ext_pos != std::string::npos)
        {
            std::string ext = full_path.substr(ext_pos + 1);
            auto it = MIME_TYPES.find(ext);
            if (it != MIME_TYPES.end())
            {
                mime_type = it->second;
            }
        }

        // 使用流式接口构建响应
        return HttpResponse::Ok()
            .withBody(content, mime_type)
            .withHeader("Cache-Control", "public, max-age=3600");
    }
}