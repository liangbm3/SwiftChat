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
        static_dir_ = "./static"; // 设置静态文件目录

        // 忽略SIGPIPE信号，避免写入已关闭的套接字导致程序终止
        signal(SIGPIPE, SIG_IGN);

        // 创建套接字
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ < 0)
        {
            LOG_ERROR << "Failed to create socket: " << strerror(errno);
            throw std::runtime_error("Failed to create socket");
        }

        // 设置套接字选项
        int opt = 1;
        // 允许服务器在关闭后立即重启，即使之前的连接还处于TIME_WAIT状态，否则会绑定失败
        if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            LOG_ERROR << "Failed to set socket options: " << strerror(errno);
            close(server_fd_);
            throw std::runtime_error("Failed to set socket options");
        }

        // 绑定套接字到指定端口
        struct sockaddr_in server_addr;
        std::memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;         // IPv4
        server_addr.sin_addr.s_addr = INADDR_ANY; // 绑定到所有可用地址
        server_addr.sin_port = htons(port_);      // 转换端口号为网络字节序
        if (bind(server_fd_, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            LOG_ERROR << "Failed to bind socket: " << strerror(errno);
            close(server_fd_);
            throw std::runtime_error("Failed to bind socket");
        }

        // 开始监听
        // SOMAXCONN 是一个宏定义，表示通过 listen 函数可指定的最大队列长度，其值为 4096。
        if (listen(server_fd_, SOMAXCONN) < 0)
        {
            LOG_ERROR << "Failed to listen on socket: " << strerror(errno);
            close(server_fd_);
            throw std::runtime_error("Failed to listen on socket");
        }
    }

    HttpServer::~HttpServer()
    {
        stop();
        if (server_fd_ >= 0)
            close(server_fd_);
    }

    void HttpServer::addHandler(const Route &route)
    {
        routes_.push_back(route);
    }

    void HttpServer::setMiddleware(Middleware middleware)
    {
        this->middleware_ = std::move(middleware);
    }

    void HttpServer::setStaticDirectory(const std::string &dir)
    {
        static_dir_ = dir;
    }

    void HttpServer::run()
    {
        running_ = true;
        LOG_INFO << "HTTP server is running on port " << port_;

        while (running_)
        {
            sockaddr_in client_addr{};
            socklen_t client_addr_len = sizeof(client_addr);
            // 接受客户端连接
            int client_fd = accept(server_fd_, (struct sockaddr *)&client_addr, &client_addr_len);
            if (client_fd < 0)
            {
                if (errno == EINTR || !running_)
                {
                    // 被中断或服务器停止
                    break;
                }
                LOG_ERROR << "Failed to accept client connection: " << strerror(errno);
                continue; // 继续等待下一个连接
            }

            // 设置客户端套接字超时
            struct timeval timeout;
            timeout.tv_sec = 30; // 30秒超时
            timeout.tv_usec = 0;
            setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
            setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

            LOG_INFO << "Accepted connection from " << inet_ntoa(client_addr.sin_addr)
                     << ":" << ntohs(client_addr.sin_port) << " (fd: " << client_fd << ")";
            thread_pool_.enqueue([this, client_fd]()
                                 {
                handleClient(client_fd); // 处理客户端连接
                return 0; });
        }
    }

    void HttpServer::stop()
    {
        running_ = false;
        if (server_fd_ != -1)
        {
            // 关闭服务器套接字以中断accept()调用
            shutdown(server_fd_, SHUT_RDWR);
            close(server_fd_);
            server_fd_ = -1;
        }
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
            // 添加CORS头和自定义响应头
            response.withHeader("Access-Control-Allow-Origin", "*")
                .withHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
                .withHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With")
                .withHeader("X-Server", "SwiftChat/1.0");
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
        // 处理所有 OPTIONS 请求（CORS 预检）
        if (request.getMethod() == "OPTIONS")
        {
            LOG_INFO << "Handling CORS preflight request for: " << request.getPath();
            return HttpResponse::Ok()
                .withHeader("Access-Control-Allow-Origin", "*")
                .withHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
                .withHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With")
                .withHeader("Access-Control-Max-Age", "86400") // 缓存24小时
                .withBody("", "text/plain");
        }
        // 遍历注册的所有路由
        for (const auto &route : routes_)
        {
            // 检查请求方法和路径是否匹配
            if (route.method == request.getMethod() && route.path == request.getPath())
            {
                // 检查这个路由是否需要验证
                if (route.use_auth_middleware && middleware_)
                {
                    // 使用中间件处理请求
                    return middleware_(request, route.handler);
                }
                else
                {
                    // 直接调用处理函数
                    return route.handler(request);
                }
            }
        }
        // 如果没有API路由匹配，尝试作为静态文件请求处理
        if (request.getMethod() == "GET" && !static_dir_.empty())
        {
            return serveStaticFile(request.getPath());
        }

        // 如果没有匹配的路由和静态文件，返回404
        return HttpResponse::NotFound("Endpoint not found");
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