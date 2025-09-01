#include "connection.hpp"

#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>

#include "http_server.hpp"
#include "router.hpp"
#include "utils/base64.hpp"
#include "utils/logger.hpp"
#include "utils/sha1.hpp"

Connection::Connection(int fd, http::HttpServer *server, http::Router *router)
    : fd_(fd), server_(server), router_(router), state_(State::HTTP) {
  LOG_DEBUG << "New connection created with fd: " << fd_;
}

Connection::~Connection() {
  LOG_DEBUG << "Connection with fd " << fd_ << " destroyed.";
  if (fd_ >= 0) {
    close(fd_);
    fd_ = -1;
  }
}

int Connection::getFd() const { return fd_; }

// 任务的入口，负责从socket读取数据并分发
void Connection::handleEvent() {
  std::lock_guard<std::mutex> lock(mutex_);
  char buffer[8192];
  const size_t MAX_BUFFER_SIZE = 1024 * 1024;  // 1MB限制

  while (true) {
    ssize_t byte_read = recv(fd_, buffer, sizeof(buffer), 0);
    if (byte_read > 0) {
      // 检查缓冲区大小限制
      if (read_buffer_.size() + static_cast<size_t>(byte_read) >
          MAX_BUFFER_SIZE) {
        LOG_WARN << "Request too large, closing connection. FD: " << fd_;
        state_ = State::CLOSING;
        break;
      }
      read_buffer_.append(buffer, static_cast<size_t>(byte_read));
    } else if (byte_read == 0) {
      LOG_DEBUG << "Connection closed by peer.";
      state_ = State::CLOSING;
      break;
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // 数据已经读完，连接不用关闭，退出循环即可
        break;
      }
      // 走到这里说明出现了其他错误
      LOG_ERROR << "Recv error on socket " << fd_ << ": " << strerror(errno);
      state_ = State::CLOSING;
      break;
    }
  }
  if (state_ == State::CLOSING) {
    return;
  }

  // 根据当前的状态处理已经读取的数据
  if (state_ == State::HTTP) {
    processHttpData();
  } else if (state_ == State::WEBSOCKET) {
    processWebSocketData();
  }

  // 如果处理完后连接还活着，重新加入epoll监听
  if (state_ != State::CLOSING) {
    server_->getEpoller().addFd(fd_, EPOLLIN | EPOLLET | EPOLLRDHUP);
  }
}

void Connection::processHttpData() {
  auto request_opt = http::HttpRequest::parse(read_buffer_);
  if (!request_opt) {
    // 解析失败，返回400 Bad Request
    http::HttpResponse response =
        http::HttpResponse::BadRequest("Invalid HTTP request format.");
    response.withHeader("Access-Control-Allow-Origin", "*")
        .withHeader("X-Server", "SwiftChat/1.0");
    std::string response_str = response.toString();
    sendResponse(response_str);
    read_buffer_.clear();  // 清空缓冲区
    state_ = State::CLOSING;
    server_->removeConnection(fd_);
    return;
  }

  read_buffer_.clear();  // 清空读取缓冲区，准备处理下一个请求
  const http::HttpRequest &request = *request_opt;

  // 检查websocket升级
  if (isWebSocketUpgradeRequest(request)) {
    if (handleWebSocketHandshake(fd_, request)) {
      state_ = State::WEBSOCKET;  // 切换到WebSocket状态
      return;                     // 握手成功，退出处理
    } else {
      state_ = State::CLOSING;  // 握手失败，关闭连接
    }
  } else {
    // 将请求委托给路由器处理
    http::HttpResponse response = router_->route(request);
    // 添加通用头部
    response.withHeader("Access-Control-Allow-Origin", "*")
        .withHeader("X-Server", "SwiftChat/1.0");
    // 发送响应
    std::string response_str = response.toString();
    if (!sendResponse(response_str)) {
      LOG_ERROR << "Failed to send HTTP response";
      state_ = State::CLOSING;
    } else {
      // 默认短连接，关闭连接
      state_ = State::CLOSING;
    }
  }
  if (state_ == State::CLOSING) {
    server_->removeConnection(fd_);
  }
}

void Connection::processWebSocketData() {
  // 循环，直到缓冲区的数据不足以解析一个完整的帧
  while(true){
    if(read_buffer_.size() < 2){
      // 至少需要2 字节的头部
      break;
    }
    
    // 解析帧头部
    // 第一个字节 FIN，RSV，Opcode
    const uint8_t byte1 = static_cast<uint8_t>(read_buffer_[0]);
    const uint8_t opcode = byte1 & 0x0F; //取后四位
    const bool fin = (byte1 & 0x80) != 0; // 检查FIN位
    
    //第二个字节：MASK，RSV，Opcode
    const uint8_t byte2 = static_cast<uint8_t>(read_buffer_[1]);
    const bool has_mask = (byte2 & 0x80) != 0;  // 检查第一个位是否为1
    uint64_t payload_length = byte2 & 0x7F;    // 取后7位

    size_t header_len = 2;

    //解析负载长度
    if(payload_length==126){
      if(read_buffer_.size()<4) break; // 扩展长度数据不完整，等待更多数据
      payload_length=(static_cast<uint8_t>(read_buffer_[2])<<8) |
                     static_cast<uint8_t>(read_buffer_[3]);
      header_len+=2;
    }else if(payload_length==127){
      if(read_buffer_.size()<10) break; // 扩展长度数据不完整，等待更多数据
      payload_length=0;
      for(int i=0;i<8;++i){
        payload_length=(payload_length<<8) | static_cast<uint8_t>(read_buffer_[2+i]);
      }
      header_len+=8;
    }

    // 检查负载长度是否合理（防止过大的帧攻击）
    const size_t MAX_FRAME_SIZE = 1024 * 1024; // 1MB
    if(payload_length > MAX_FRAME_SIZE) {
      LOG_WARN << "WebSocket frame too large (" << payload_length << " bytes), closing connection";
      state_ = State::CLOSING;
      break;
    }

    // 检查掩码和负载数据是否完整
    const size_t masking_key_len = has_mask ? 4 : 0;
    const size_t total_frame_size = header_len + masking_key_len + payload_length;
    if(read_buffer_.size() < total_frame_size){
      // 数据不完整，等待更多数据
      break;
    }

    //提取掩码和负载
    std::string masking_key;
    if(has_mask){
      masking_key = read_buffer_.substr(header_len, masking_key_len);
    }

    std::string payload = read_buffer_.substr(header_len + masking_key_len, payload_length);

    //解码负载
    if(has_mask){
      for(size_t i=0;i<payload.length();++i){
        payload[i] ^= masking_key[i%4];
      }
    }

    // 根据Opcode处理帧
    bool frame_processed = true;
    switch (opcode)
    {
    case 0x0:  // 连续帧
      LOG_DEBUG << "WebSocket (fd: " << fd_ << ") received continuation frame";
      // TODO: 实现分片消息处理
      break;
    case 0x1:  // 文本帧
      LOG_INFO << "WebSocket (fd: " << fd_ << ") received text frame: " << payload;
      sendWebSocketFrame(payload);
      break;
    case 0x2:  // 二进制帧
      LOG_INFO << "WebSocket (fd: " << fd_ << ") received binary frame";
      // 发送不支持的帧类型错误，而不是直接关闭连接
      sendWebSocketFrame("Binary frames not supported", 0x1);
      break;
    case 0x8:  // 关闭帧
      LOG_INFO << "WebSocket (fd: " << fd_ << ") received close frame";
      // 响应关闭帧并关闭连接
      sendWebSocketFrame("", 0x8);
      state_ = State::CLOSING;
      frame_processed = false; // 不继续处理更多帧
      break;
    case 0x9:  // Ping 帧
      LOG_DEBUG << "WebSocket (fd: " << fd_ << ") received ping frame";
      // 响应 Pong 帧
      sendWebSocketFrame(payload, 0xA);
      break;
    case 0xA:  // Pong 帧
      LOG_DEBUG << "WebSocket (fd: " << fd_ << ") received pong frame";
      // Pong帧通常用于心跳响应，这里只记录日志
      break;
    default:
      LOG_WARN << "WebSocket (fd: " << fd_ << ") received unknown opcode: " << static_cast<int>(opcode);
      // 对于未知帧类型，记录警告但不关闭连接
      break;
    }

    // 从缓冲区中移除已处理的帧数据
    read_buffer_.erase(0, total_frame_size);

    // 如果收到关闭帧，停止处理更多帧
    if (!frame_processed) {
      break;
    }
  }
}

// 发送响应的辅助方法，处理部分发送的情况
bool Connection::sendResponse(const std::string &response) {
  const char *data = response.c_str();
  size_t total_bytes = response.length();
  size_t sent_bytes = 0;

  while (sent_bytes < total_bytes) {
    ssize_t result =
        send(fd_, data + sent_bytes, total_bytes - sent_bytes, MSG_NOSIGNAL);
    if (result < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
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
bool Connection::isWebSocketUpgradeRequest(const http::HttpRequest &request) {
  // 检查必需的 WebSocket 头部
  auto connection = request.getHeaderValue("Connection");
  auto upgrade = request.getHeaderValue("Upgrade");
  auto websocket_key = request.getHeaderValue("Sec-WebSocket-Key");
  auto websocket_version = request.getHeaderValue("Sec-WebSocket-Version");

  // 检查是否包含必需的头部
  if (!connection || !upgrade || !websocket_key || !websocket_version) {
    return false;
  }

  // 转换为小写进行比较
  std::string connection_str(*connection);
  std::string upgrade_str(*upgrade);
  std::transform(connection_str.begin(), connection_str.end(),
                 connection_str.begin(), ::tolower);
  std::transform(upgrade_str.begin(), upgrade_str.end(), upgrade_str.begin(),
                 ::tolower);

  // 检查是否包含必需的头部
  return request.getMethod() == "GET" &&
         connection_str.find("upgrade") != std::string::npos &&
         upgrade_str == "websocket" && std::string(*websocket_version) == "13";
}

// WebSocket 握手处理
bool Connection::handleWebSocketHandshake(int client_fd,
                                          const http::HttpRequest &request) {
  try {
    auto websocket_key_opt = request.getHeaderValue("Sec-WebSocket-Key");
    if (!websocket_key_opt) {
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
        accept_key +
        "\r\n"
        "\r\n";

    // 发送握手响应
    if (!sendResponse(response)) {
      LOG_ERROR << "Failed to send WebSocket handshake response: "
                << strerror(errno);
      return false;
    }

    LOG_INFO << "WebSocket handshake completed successfully for fd "
             << client_fd;
    return true;
  } catch (const std::exception &e) {
    LOG_ERROR << "Exception in WebSocket handshake: " << e.what();
    return false;
  }
}
// 生成 WebSocket Accept Key
std::string Connection::generateWebSocketAcceptKey(
    const std::string &websocket_key) {
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

void Connection::closeConnection(){
  if(state_==State::CLOSING) return; //已经在关闭状态
  LOG_DEBUG << "Closing connection for fd " << fd_;
  state_ = State::CLOSING;
  // 关闭套接字等清理工作
  server_->removeConnection(fd_);
}


void Connection::sendWebSocketFrame(const std::string &message, uint8_t opcode) {
    std::string frame;
    frame += static_cast<char>(0x80 | opcode); // FIN=1, RSV=0

    const size_t payload_len = message.length();
    if (payload_len <= 125) {
        frame += static_cast<char>(payload_len);
    } else if (payload_len <= 65535) {
        frame += static_cast<char>(126);
        frame += static_cast<char>((payload_len >> 8) & 0xFF);
        frame += static_cast<char>(payload_len & 0xFF);
    } else {
        frame += static_cast<char>(127);
        for (int i=7; i>=0; --i) {
            frame += static_cast<char>((payload_len >> (i*8)) & 0xFF);
        }
    }

    frame += message;

    // 使用你已经实现的sendResponse来发送帧数据
    if (!sendResponse(frame)) {
        LOG_ERROR << "Failed to send WebSocket frame to fd " << fd_;
        state_ = State::CLOSING;
    } else {
        LOG_INFO << "WebSocket (fd " << fd_ << ") sent: " << message;
    }
}