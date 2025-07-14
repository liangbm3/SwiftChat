#include "http_server.hpp"
#include "utils/logger.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <cerrno>
#include <fcntl.h>
#include <signal.h>

namespace http
{
    HttpServer::HttpServer(int port)
        : port_(port), running_(false), thread_pool_(std::thread::hardware_concurrency()), static_dir_("")
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
        {
            close(server_fd_);
        }
    }

    void HttpServer::addHandler(const std::string &path, const std::string &method, RequestHandler handler)
    {
        handlers_[path][method] = std::move(handler);
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
                if (errno == EINTR || !running_) {
                    // 被中断或服务器停止
                    break;
                }
                LOG_ERROR << "Failed to accept client connection: " << strerror(errno);
                continue; // 继续等待下一个连接
            }
            
            // 设置客户端套接字超时
            struct timeval timeout;
            timeout.tv_sec = 30;  // 30秒超时
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

    void HttpServer::handleClient(int client_fd)
    {
        try {
            const size_t BUFFER_SIZE = 8192;
            std::string request_data;
            char buffer[BUFFER_SIZE];
            
            // 读取HTTP请求，可能需要多次读取
            while (true) {
                ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                if (bytes_received > 0) {
                    buffer[bytes_received] = '\0';
                    request_data.append(buffer, bytes_received);
                    
                    // 检查是否接收到完整的HTTP请求头
                    if (request_data.find("\r\n\r\n") != std::string::npos) {
                        break;
                    }
                    
                    // 防止请求过大
                    if (request_data.size() > 1024 * 1024) { // 1MB限制
                        LOG_WARN << "Request too large, closing connection";
                        close(client_fd);
                        return;
                    }
                } else if (bytes_received == 0) {
                    LOG_INFO << "Client disconnected (fd: " << client_fd << ")";
                    close(client_fd);
                    return;
                } else {
                    if (errno == EWOULDBLOCK || errno == EAGAIN) {
                        LOG_WARN << "Client timeout (fd: " << client_fd << ")";
                    } else {
                        LOG_ERROR << "Failed to receive data: " << strerror(errno);
                    }
                    close(client_fd);
                    return;
                }
            }

            HttpRequest request = HttpRequest::parse(request_data); // 解析HTTP请求
            HttpResponse response;

            LOG_INFO << "Request: " << request.method << " " << request.path
                     << " (Content-Length: " << (request.headers.count("Content-Length") ? request.headers.at("Content-Length") : "0") << ")";
            LOG_DEBUG << "Request body: " << request.body;

            // 查找对应路径的处理函数
            auto path_it = handlers_.find(request.path);
            if (path_it != handlers_.end()) // 如果找到
            {
                // 查找对应方法的处理函数
                auto method_it = path_it->second.find(request.method);
                if (method_it != path_it->second.end())
                {
                    LOG_DEBUG << "Found handler for " << request.method << " " << request.path;
                    try {
                        response = method_it->second(request); // 调用处理函数
                    } catch (const std::exception& e) {
                        LOG_ERROR << "Handler exception: " << e.what();
                        response = HttpResponse(500, "{\"status\":\"error\",\"message\":\"Internal server error\"}");
                    }
                }
                else
                {
                    response = HttpResponse(405, "{\"status\":\"error\",\"message\":\"Method not allowed\"}"); // 方法不允许
                    LOG_WARN << "Method not allowed: " << request.method << " for path " << request.path;
                }
            }
            else if (request.path == "/" || request.path.find('.') != std::string::npos) // 如果是静态文件请求
            {
                std::string path = request.path == "/" ? "/index.html" : request.path;
                std::string full_path = static_dir_ + path;
                LOG_DEBUG << "Serving static file: " << full_path;
                serveStaticFile(full_path, response); // 提供静态文件服务
            }
            else
            {
                LOG_WARN << "Not found: " << request.path;
                response = HttpResponse(404, "{\"status\":\"error\",\"message\":\"Not found\"}");
            }
            
            // 添加跨域资源共享头和安全头
            response.headers["Access-Control-Allow-Origin"] = "*";
            response.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS";
            response.headers["Access-Control-Allow-Headers"] = "Content-Type";

            std::string response_str = response.toString();
            ssize_t total_bytes_sent = 0;
            const char *response_data = response_str.c_str();
            size_t remaining_bytes = response_str.length();
            
            // 发送响应数据到客户端
            while (remaining_bytes > 0)
            {
                ssize_t bytes_sent = send(client_fd, response_data + total_bytes_sent, remaining_bytes, MSG_NOSIGNAL);
                if (bytes_sent < 0)
                {
                    if (errno == EPIPE || errno == ECONNRESET) {
                        LOG_INFO << "Client disconnected during send (fd: " << client_fd << ")";
                    } else {
                        LOG_ERROR << "Failed to send response: " << strerror(errno);
                    }
                    break; // 发送失败，退出循环
                }
                total_bytes_sent += bytes_sent;
                remaining_bytes -= bytes_sent;
            }
            LOG_DEBUG << "Sent " << total_bytes_sent << " bytes to client";
            
        } catch (const std::exception& e) {
            LOG_ERROR << "Exception in handleClient: " << e.what();
        }
        
        // 关闭客户端套接字
        close(client_fd);
    }
    void HttpServer::serveStaticFile(const std::string &path, HttpResponse &response)
    {
        struct stat file_stat; // 用来获取文件的元信息
        if (stat(path.c_str(), &file_stat) < 0)
        {
            LOG_ERROR << "Failed to stat file: " << path << " - " << strerror(errno);
            response = HttpResponse(404, "File not found");
            return;
        }

        std::string ext = path.substr(path.find_last_of('.') + 1); // 确定文件的扩展名

        // 设置Content-Type头
        const std::unordered_map<std::string, std::string> mimeTypes = {
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
            {"txt", "text/plain"},
            {"pdf", "application/pdf"},
            {"zip", "application/zip"},
            {"woff", "font/woff"},
            {"woff2", "font/woff2"},
            {"ttf", "font/ttf"},
            {"eot", "application/vnd.ms-fontobject"},
            {"mp3", "audio/mpeg"},
            {"mp4", "video/mp4"},
            {"webm", "video/webm"},
            {"webp", "image/webp"}};
        auto mime_it = mimeTypes.find(ext);
        if (mime_it != mimeTypes.end())
        {
            response.headers["Content-Type"] = mime_it->second;
        }
        else
        {
            response.headers["Content-Type"] = "application/octet-stream"; // 默认二进制流
        }

        // 读取文件内容
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            LOG_ERROR << "Failed to open static file: " << path << " - " << strerror(errno);
            response = HttpResponse(500, "Internal Server Error");
            return;
        }

        // 获取文件大小
        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        // 对于大文件，限制大小或使用流式传输
        const size_t MAX_FILE_SIZE = 50 * 1024 * 1024; // 50MB
        if (file_size > MAX_FILE_SIZE) {
            LOG_ERROR << "File too large: " << path << " (size: " << file_size << ")";
            response = HttpResponse(413, "File too large");
            return;
        }

        // 读取文件内容并设置响应体
        std::string file_content;
        file_content.resize(file_size);
        file.read(&file_content[0], file_size);
        
        response.status_code = 200;   // 设置状态码为200 OK
        response.body = std::move(file_content); // 设置响应体为文件内容

        // 添加静态文件的缓存策略
        response.headers["Cache-Control"] = "public, max-age=3600"; // 缓存1小时

        // 添加安全头
        response.headers["X-Content-Type-Options"] = "nosniff";
        response.headers["X-Frame-Options"] = "SAMEORIGIN";
        response.headers["X-XSS-Protection"] = "1; mode=block";

        // 添加CORS头
        response.headers["Access-Control-Allow-Origin"] = "*";
        response.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS";
        response.headers["Access-Control-Allow-Headers"] = "Content-Type";
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

    void HttpServer::setStaticDirectory(const std::string &dir)
    {
        static_dir_ = dir;
    }
}