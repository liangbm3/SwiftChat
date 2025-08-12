#include "connection.hpp"
#include "http_server.hpp"
#include "router.hpp"
#include "utils/logger.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <algorithm>
#include <cctype>
#include "utils/base64.hpp"
#include "utils/sha1.hpp"

Connection::Connection(int fd, http::HttpServer *server, http::Router *router)
    : fd_(fd), server_(server), router_(router), state_(State::HTTP)
{
    LOG_DEBUG << "New connection created with fd: " << fd_;
}

Connection::~Connection()
{
    LOG_DEBUG << "Connection with fd " << fd_ << " destroyed.";
    if (fd_ >= 0)
    {
        close(fd_);
        fd_ = -1;
    }
}

int Connection::getFd() const
{
    return fd_;
}

// 任务的入口，负责从socket读取数据并分发
void Connection::handleEvent()
{
    std::lock_guard<std::mutex> lock(mutex_);
    char buffer[8192];
    const size_t MAX_BUFFER_SIZE = 1024 * 1024; // 1MB限制
    
    while (true)
    {
        ssize_t byte_read = recv(fd_, buffer, sizeof(buffer), 0);
        if (byte_read > 0)
        {
            // 检查缓冲区大小限制
            if (read_buffer_.size() + static_cast<size_t>(byte_read) > MAX_BUFFER_SIZE)
            {
                LOG_WARN << "Request too large, closing connection. FD: " << fd_;
                state_ = State::CLOSING;
                break;
            }
            read_buffer_.append(buffer, static_cast<size_t>(byte_read));
        }
        else if (byte_read == 0)
        {
            LOG_DEBUG << "Connection closed by peer.";
            state_ = State::CLOSING;
            break;
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 数据已经读完，连接不用关闭，退出循环即可
                break;
            }
            // 走到这里说明出现了其他错误
            LOG_ERROR << "Recv error on socket " << fd_ << ": " << strerror(errno);
            state_ = State::CLOSING;
            break;
        }
    }
    if (state_ == State::CLOSING)
    {
        // 通知服务器移除自身
        server_->removeConnection(fd_);
        return;
    }

    // 根据当前的状态处理已经读取的数据
    if (state_ == State::HTTP)
    {
        processHttpData();
    }
    else if (state_ == State::WEBSOCKET)
    {
        processWebSocketData();
    }

    // 如果处理完后连接还活着，重新加入epoll监听
    if (state_ != State::CLOSING)
    {
        server_->getEpoller().addFd(fd_, EPOLLIN | EPOLLET | EPOLLRDHUP);
    }
}

void Connection::processHttpData()
{
    auto request_opt = http::HttpRequest::parse(read_buffer_);
    if (!request_opt)
    {
        // 解析失败，返回400 Bad Request
        http::HttpResponse response = http::HttpResponse::BadRequest("Invalid HTTP request format.");
        response.withHeader("Access-Control-Allow-Origin", "*")
            .withHeader("X-Server", "SwiftChat/1.0");
        std::string response_str = response.toString();
        sendResponse(response_str);
        read_buffer_.clear(); // 清空缓冲区
        state_ = State::CLOSING;
        server_->removeConnection(fd_);
        return;
    }

    read_buffer_.clear(); // 清空读取缓冲区，准备处理下一个请求
    http::HttpRequest &request = *request_opt;

    // 检查websocket升级
    if (isWebSocketUpgradeRequest(request))
    {
        if (handleWebSocketHandshake(fd_, request))
        {
            state_ = State::WEBSOCKET; // 切换到WebSocket状态
            return;                    // 握手成功，退出处理
        }
        else
        {
            state_ = State::CLOSING; // 握手失败，关闭连接
        }
    }
    else
    {
        // 将请求委托给路由器处理
        http::HttpResponse response = router_->route(request);
        // 添加通用头部
        response.withHeader("Access-Control-Allow-Origin", "*")
            .withHeader("X-Server", "SwiftChat/1.0");
        // 发送响应
        std::string response_str = response.toString();
        if (!sendResponse(response_str))
        {
            LOG_ERROR << "Failed to send HTTP response";
            state_ = State::CLOSING;
        }
        else
        {
            // 默认短连接，关闭连接
            state_ = State::CLOSING;
        }
    }
    if (state_ == State::CLOSING)
    {
        server_->removeConnection(fd_);
    }
}

void Connection::processWebSocketData()
{
    // 处理WebSocket数据帧
}

// 发送响应的辅助方法，处理部分发送的情况
bool Connection::sendResponse(const std::string &response)
{
    const char *data = response.c_str();
    size_t total_bytes = response.length();
    size_t sent_bytes = 0;

    while (sent_bytes < total_bytes)
    {
        ssize_t result = send(fd_, data + sent_bytes, total_bytes - sent_bytes, MSG_NOSIGNAL);
        if (result < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 发送缓冲区满，稍后重试（在实际项目中可能需要epoll EPOLLOUT）
                continue;
            }
            LOG_ERROR << "Send error on socket " << fd_ << ": " << strerror(errno);
            return false;
        }
        sent_bytes += static_cast<size_t>(result);
    }
    return true;
}

// WebSocket 升级请求检测
bool Connection::isWebSocketUpgradeRequest(const http::HttpRequest &request)
{
    // 检查必需的 WebSocket 头部
    auto connection = request.getHeaderValue("Connection");
    auto upgrade = request.getHeaderValue("Upgrade");
    auto websocket_key = request.getHeaderValue("Sec-WebSocket-Key");
    auto websocket_version = request.getHeaderValue("Sec-WebSocket-Version");

    // 检查是否包含必需的头部
    if (!connection || !upgrade || !websocket_key || !websocket_version)
    {
        return false;
    }

    // 转换为小写进行比较
    std::string connection_str(*connection);
    std::string upgrade_str(*upgrade);
    std::transform(connection_str.begin(), connection_str.end(), connection_str.begin(), ::tolower);
    std::transform(upgrade_str.begin(), upgrade_str.end(), upgrade_str.begin(), ::tolower);

    // 检查是否包含必需的头部
    return request.getMethod() == "GET" &&
           connection_str.find("upgrade") != std::string::npos &&
           upgrade_str == "websocket" &&
           std::string(*websocket_version) == "13";
}

// WebSocket 握手处理
bool Connection::handleWebSocketHandshake(int client_fd, const http::HttpRequest &request)
{
    try
    {
        auto websocket_key_opt = request.getHeaderValue("Sec-WebSocket-Key");
        if (!websocket_key_opt)
        {
            LOG_ERROR << "Missing Sec-WebSocket-Key header";
            return false;
        }

        std::string websocket_key(*websocket_key_opt);

        // WebSocket 握手响应
        std::string accept_key = generateWebSocketAcceptKey(websocket_key);

        std::string response =
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: " +
            accept_key + "\r\n"
                         "\r\n";

        // 发送握手响应
        if (!sendResponse(response))
        {
            LOG_ERROR << "Failed to send WebSocket handshake response: " << strerror(errno);
            return false;
        }

        LOG_INFO << "WebSocket handshake completed successfully for fd " << client_fd;
        return true;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Exception in WebSocket handshake: " << e.what();
        return false;
    }
}
// 生成 WebSocket Accept Key
std::string Connection::generateWebSocketAcceptKey(const std::string &websocket_key)
{
    // WebSocket 规范中定义的魔法字符串
    const std::string WEBSOCKET_MAGIC = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    // 连接 WebSocket-Key 和魔法字符串
    std::string combined = websocket_key + WEBSOCKET_MAGIC;

    // 计算 SHA1 散列
    SHA1 sha1;
    sha1.update(combined);
    std::vector<uint8_t> sha1_hash = sha1.final();

    // Base64 编码
    return base64_encode(sha1_hash);
}