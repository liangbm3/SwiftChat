#include "http_server.hpp"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include "utils/logger.hpp"

namespace http
{
    // 初始化静态MIME类型映射表
    const std::unordered_map<std::string, std::string> HttpServer::MIME_TYPES = {
        {"html", "text/html"},
        {"css", "text/css"},
        {"js", "application/javascript"},
        {"json", "application/json"},
        {"png", "image/png"},
        {"jpg", "image/jpeg"},
        {"jpeg", "image/jpeg"},
        {"gif", "image/gif"},
        {"svg", "image/svg+xml"},
        {"ico", "image/x-icon"},
        {"txt", "text/plain"}};

    HttpServer::HttpServer(int port, size_t thread_count)
        : port_(port),
          running_(false),
          thread_pool_(thread_count),
          epoller_(),
          static_dir_("./static")
    {
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

        // 性能优化：设置套接字缓冲区大小
        int send_buffer = 65536;    // 64KB发送缓冲区
        int recv_buffer = 65536;    // 64KB接收缓冲区
        setsockopt(server_fd_, SOL_SOCKET, SO_SNDBUF, &send_buffer, sizeof(send_buffer));
        setsockopt(server_fd_, SOL_SOCKET, SO_RCVBUF, &recv_buffer, sizeof(recv_buffer));

        // 启用TCP_NODELAY，禁用Nagle算法以减少延迟
        setsockopt(server_fd_, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

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

        // 开始监听连接
        if (listen(server_fd_, SOMAXCONN) < 0)
        {
            LOG_ERROR << "Failed to listen on socket: " << strerror(errno);
            close(server_fd_);
            throw std::runtime_error("Failed to listen on socket");
        }

        setNoBlocking(server_fd_); // 设置非阻塞模式
        // 将监听套接字添加到epoll中，监听读事件，使用ET
        if (!epoller_.addFd(server_fd_, EPOLLIN | EPOLLET))
        {
            LOG_ERROR << "Failed to add server socket to epoll: " << strerror(errno);
            close(server_fd_);
            throw std::runtime_error("Failed to add server socket to epoll");
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
            // 等待epoll事件（设置1秒超时，以便能够响应关闭信号）
            int event_count = epoller_.wait(1000); // 1000ms超时
            if (event_count < 0)
            {
                if (errno == EINTR)
                {
                    continue; // 被信号中断，继续等待
                }
                LOG_ERROR << "Epoll wait error: " << strerror(errno);
                break; // 其他错误，退出循环
            }
            else if (event_count == 0)
            {
                // 超时，没有事件，继续循环（这会检查running_标志）
                continue;
            }
            // 遍历所有就绪事件
            for (int i = 0; i < event_count; i++)
            {
                int fd = epoller_.getEventFd(i);
                uint32_t events = epoller_.getEvents(i);
                if (fd == server_fd_)
                {
                    // 新连接到达
                    // ET模式需要循环accept直到没有连接
                    while (true)
                    {
                        sockaddr_in client_addr{};
                        socklen_t client_addr_len = sizeof(client_addr);
                        int client_fd =
                            accept(server_fd_, (struct sockaddr *)&client_addr, &client_addr_len);
                        if (client_fd < 0)
                        {
                            if (errno == EAGAIN || errno == EWOULDBLOCK)
                            {
                                break; // 没有更多连接，退出循环
                            }
                            LOG_ERROR << "Failed to accept connection: " << strerror(errno);
                            break;
                        }
                        
                        // 优化：减少日志输出，避免DNS查找
                        LOG_DEBUG << "Accepted new connection from " 
                                 << ((client_addr.sin_addr.s_addr >> 0) & 0xFF) << "."
                                 << ((client_addr.sin_addr.s_addr >> 8) & 0xFF) << "."
                                 << ((client_addr.sin_addr.s_addr >> 16) & 0xFF) << "."
                                 << ((client_addr.sin_addr.s_addr >> 24) & 0xFF)
                                 << ":" << ntohs(client_addr.sin_port);
                        
                        // 为客户端连接设置性能优化选项
                        int opt = 1;
                        setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
                        
                        // 将新客户端设置为非阻塞，并添加到epoll中
                        setNoBlocking(client_fd);
                        epoller_.addFd(client_fd,
                                       EPOLLIN | EPOLLET | EPOLLRDHUP); // 监听读事件和连接关闭事件
                    }
                }
                else
                {
                    // 处理客户端套接字事件
                    if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
                    {
                        // 错误或连接关闭
                        LOG_INFO << "Client fd " << fd << " closed or error";
                        epoller_.removeFd(fd);
                        close(fd);
                    }
                    else if (events & EPOLLIN)
                    {
                        // 有数据可读，从epoller中移除并交给线程池处理
                        epoller_.removeFd(fd);
                        thread_pool_.enqueue([this, fd]()
                                             { handleClient(fd); });
                    }
                    else
                    {
                        LOG_WARN << "Unhandled epoll event for fd " << fd << ": " << events;
                    }
                }
            }
        }
        
        LOG_INFO << "HTTP server main loop exited";
    }

    void HttpServer::stop()
    {
        running_ = false;
        if (server_fd_ >= 0)
        {
            // 关闭服务器套接字以中断accept()调用
            if (shutdown(server_fd_, SHUT_RDWR) < 0)
            {
                LOG_WARN << "Failed to shutdown server socket: " << strerror(errno);
            }
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
            // 非阻塞循环读取，直到缓冲区为空
            while (true)
            {
                ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
                if (bytes_received > 0)
                {
                    request_data.append(buffer, bytes_received);
                }
                else if (bytes_received == 0)
                {
                    LOG_INFO << "Client fd " << client_fd << " disconnected.";
                    close(client_fd);
                    return; // 客户端已关闭连接
                }
                else
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        // 没有更多数据可读，退出循环
                        break;
                    }
                    LOG_ERROR << "recv error on fd " << client_fd << ": " << strerror(errno);
                    close(client_fd);
                    return; // 发生错误，关闭连接
                }
            }
            if (request_data.empty())
            {
                LOG_WARN << "Received empty request from client fd " << client_fd;
                close(client_fd);
                return; // 没有数据，直接关闭连接
            }

            // [适配] 使用新的 HttpRequest API
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
                .withHeader("Access-Control-Allow-Headers",
                            "Content-Type, Authorization, X-Requested-With")
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
                .withHeader("Access-Control-Allow-Headers",
                            "Content-Type, Authorization, X-Requested-With")
                .withHeader("Access-Control-Max-Age", "86400") // 缓存24小时
                .withBody("", "text/plain");
        }
        // 遍历注册的所有路由
        for (const auto &route : routes_)
        {
            // 检查请求方法是否匹配
            if (route.method == request.getMethod())
            {
                std::unordered_map<std::string, std::string> pathParams;
                // 检查路径是否匹配（支持路径参数）
                if (matchPath(route.path, request.getPath(), pathParams))
                {
                    // 创建一个可修改的请求副本来设置路径参数
                    HttpRequest modifiableRequest = request;
                    modifiableRequest.setPathParams(pathParams);

                    // 检查这个路由是否需要验证
                    if (route.use_auth_middleware && middleware_)
                    {
                        // 使用中间件处理请求
                        return middleware_(modifiableRequest, route.handler);
                    }
                    else
                    {
                        // 直接调用处理函数
                        return route.handler(modifiableRequest);
                    }
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

    // 路径参数匹配和提取实现
    bool HttpServer::matchPath(const std::string &pattern,
                               const std::string &path,
                               std::unordered_map<std::string, std::string> &params)
    {
        params.clear();

        // 分割模式和路径
        auto splitPath = [](const std::string &str) -> std::vector<std::string>
        {
            std::vector<std::string> segments;
            std::stringstream ss(str);
            std::string segment;
            while (std::getline(ss, segment, '/'))
            {
                if (!segment.empty())
                {
                    segments.push_back(segment);
                }
            }
            return segments;
        };

        auto patternSegments = splitPath(pattern);
        auto pathSegments = splitPath(path);

        // 段数必须相同
        if (patternSegments.size() != pathSegments.size())
        {
            return false;
        }

        // 逐段匹配
        for (size_t i = 0; i < patternSegments.size(); ++i)
        {
            const std::string &patternSeg = patternSegments[i];
            const std::string &pathSeg = pathSegments[i];

            // 检查是否为参数段（以{开头并以}结尾）
            if (patternSeg.length() > 2 && patternSeg.front() == '{' && patternSeg.back() == '}')
            {
                // 提取参数名（去掉{}）
                std::string paramName = patternSeg.substr(1, patternSeg.length() - 2);
                params[paramName] = pathSeg;
            }
            else
            {
                // 精确匹配
                if (patternSeg != pathSeg)
                {
                    return false;
                }
            }
        }

        return true;
    }
    void HttpServer::setNoBlocking(int fd)
    {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1)
        {
            LOG_ERROR << "Failed to get file descriptor flags: " << strerror(errno);
            return;
        }
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            LOG_ERROR << "Failed to set file descriptor to non-blocking: " << strerror(errno);
        }
    }
}
