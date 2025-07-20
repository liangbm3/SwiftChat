# SwiftChat WebSocket API 文档

## 概述

SwiftChat WebSocket API 提供实时聊天功能，支持用户认证、房间管理和消息传递。所有通信都基于 JSON 格式的消息，响应格式与HTTP API保持一致。

## 连接信息

- **协议**: WebSocket (ws://)
- **默认端口**: 8081

## 统一响应格式

所有WebSocket响应都遵循以下格式：

### 成功响应
```json
{
  "success": true,
  "message": "操作描述",
  "data": {
    // 具体数据
  }
}
```

### 错误响应
```json
{
  "success": false,
  "message": "错误描述",
  "error": "具体错误信息"
}
```
- **连接地址**: `ws://hostname:8081`
- **消息格式**: JSON
- **认证方式**: JWT Token

## 连接流程

1. 建立 WebSocket 连接
2. 发送认证消息（必须是第一条消息）
3. 收到认证成功响应后，可以进行其他操作
4. 加入房间并开始聊天

## API 参考

### 1. 认证 (Authentication)

#### 1.1 客户端认证请求

**消息类型**: `auth`

```json
{
  "type": "auth",
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
}
```

**字段说明**:
- `type`: 消息类型，固定为 "auth"
- `token`: JWT 认证令牌（服务器将从token中解析用户身份信息）

#### 1.2 服务器认证响应

**认证成功**:
```json
{
  "success": true,
  "message": "WebSocket authentication successful",
  "data": {
    "user_id": "user123",
    "status": "connected"
  }
}
```

**认证失败**:
```json
{
  "success": false,
  "message": "Authentication failed",
  "error": "Invalid or expired token"
}
```

### 2. 房间管理 (Room Management)

#### 2.1 加入房间

**客户端请求**:
```json
{
  "type": "join_room",
  "room_id": "room456"
}
```

**服务器响应**:
```json
{
  "success": true,
  "message": "Successfully joined room",
  "data": {
    "type": "room_joined",
    "room_id": "room456",
    "user_id": "user123"
  }
}
```

**其他用户收到的通知**:
```json
{
  "success": true,
  "message": "User joined room",
  "data": {
    "type": "user_joined",
    "user_id": "user123",
    "room_id": "room456"
  }
}
```

#### 2.2 离开房间

**客户端请求**:
```json
{
  "type": "leave_room"
}
```

**服务器响应**:
```json
{
  "success": true,
  "message": "Successfully left room",
  "data": {
    "type": "room_left",
    "room_id": "room456",
    "user_id": "user123"
  }
}
```

**其他用户收到的通知**:
```json
{
  "success": true,
  "message": "User left room",
  "data": {
    "type": "user_left",
    "user_id": "user123",
    "room_id": "room456"
  }
}
```

### 3. 聊天消息 (Chat Messages)

#### 3.1 发送聊天消息

**客户端请求**:
```json
{
  "type": "send_message",
  "content": "Hello, world!"
}
```

**字段说明**:
- `type`: 消息类型，固定为 "send_message"
- `content`: 消息内容（字符串）

#### 3.2 接收聊天消息

**房间内所有用户收到**:
```json
{
  "success": true,
  "message": "Message sent successfully",
  "data": {
    "type": "message_received",
    "user_id": "user123",
    "room_id": "room456",
    "content": "Hello, world!",
    "timestamp": 1642588800
  }
}
```

**字段说明**:
- `type`: 数据类型，固定为 "message_received"
- `user_id`: 发送消息的用户ID
- `room_id`: 消息所在的房间ID
- `content`: 消息内容
- `timestamp`: Unix 时间戳（秒）

### 4. 心跳机制 (Heartbeat)

#### 4.1 客户端发送心跳

```json
{
  "type": "ping"
}
```

#### 4.2 服务器响应

```json
{
  "success": true,
  "message": "Pong response",
  "data": {
    "type": "pong",
    "timestamp": 1642588800
  }
}
```

### 5. 错误处理 (Error Handling)

#### 5.1 通用错误响应

```json
{
  "success": false,
  "message": "Request failed",
  "error": "具体错误描述信息"
}
```

#### 5.2 常见错误类型

| 错误消息 | 描述 | 解决方案 |
|---------|------|----------|
| `Invalid JSON format` | JSON 格式错误 | 检查消息格式是否正确 |
| `Missing required field: room_id` | 缺少必需字段 | 确保包含所有必需字段 |
| `You must join a room before sending messages` | 未加入房间就发送消息 | 先加入房间再发送消息 |
| `You are not in any room` | 尝试离开房间但未在任何房间中 | 确认已加入房间 |
| `Invalid or expired token` | JWT 令牌无效或过期 | 重新获取有效的认证令牌 |
| `First message must be an authentication message` | 首条消息必须是认证消息 | 连接后立即发送认证消息 |

## 使用示例

### JavaScript 客户端示例

```javascript
// 建立连接
const ws = new WebSocket('ws://localhost:8081');

// 连接打开后立即认证
ws.onopen = function() {
    const authMessage = {
        type: 'auth',
        token: 'your_jwt_token_here',
        user_id: 'user123'
    };
    ws.send(JSON.stringify(authMessage));
};

// 处理接收到的消息
ws.onmessage = function(event) {
    const message = JSON.parse(event.data);
    
    switch(message.type) {
        case 'auth_success':
            console.log('认证成功');
            // 加入房间
            joinRoom('room456');
            break;
            
        case 'room_joined':
            console.log('已加入房间:', message.room_id);
            break;
            
        case 'chat_message':
            console.log(`${message.user_id}: ${message.content}`);
            break;
            
        case 'user_joined':
            console.log(`${message.user_id} 加入了房间`);
            break;
            
        case 'user_left':
            console.log(`${message.user_id} 离开了房间`);
            break;
            
        case 'error':
            console.error('错误:', message.message);
            break;
    }
};

// 加入房间
function joinRoom(roomId) {
    const message = {
        type: 'join_room',
        room_id: roomId
    };
    ws.send(JSON.stringify(message));
}

// 发送聊天消息
function sendMessage(content) {
    const message = {
        type: 'chat_message',
        content: content
    };
    ws.send(JSON.stringify(message));
}

// 离开房间
function leaveRoom() {
    const message = {
        type: 'leave_room'
    };
    ws.send(JSON.stringify(message));
}

// 发送心跳
function sendHeartbeat() {
    const message = {
        type: 'ping'
    };
    ws.send(JSON.stringify(message));
}
```

## 服务器架构

### 核心组件

- **WebSocketServer**: 主要的 WebSocket 服务器类
- **DatabaseManager**: 数据库管理器，处理消息持久化
- **JWT验证**: 基于 JWT 的用户认证机制

### 数据结构

```cpp
// 用户连接管理
std::unordered_map<std::string, connection_hdl> user_connections_;
std::unordered_map<connection_hdl, std::string> connection_users_;

// 房间管理
std::unordered_map<std::string, std::set<std::string>> room_members_;
std::unordered_map<std::string, std::string> user_current_room_;
```

### 线程安全

- 使用 `std::mutex connection_mutex_` 保护共享数据结构
- 避免在广播消息时产生死锁
- 异步处理连接断开和清理

## 最佳实践

### 客户端开发建议

1. **连接后立即认证**: 第一条消息必须是认证消息
2. **错误处理**: 监听 `error` 类型消息并适当处理
3. **心跳维持**: 定期发送 `ping` 消息保持连接活跃
4. **优雅断线**: 离开房间后再断开连接
5. **重连机制**: 实现自动重连逻辑处理网络异常

### 服务器端特性

1. **自动清理**: 用户断开连接时自动从房间移除
2. **消息持久化**: 聊天消息自动保存到数据库
3. **房间管理**: 空房间自动清理
4. **广播优化**: 支持排除特定用户的广播
5. **错误恢复**: 发送失败时自动处理断线用户

## 安全注意事项

1. **JWT 验证**: 所有连接必须通过 JWT 认证
2. **权限检查**: 用户只能在已加入的房间中发送消息
3. **输入验证**: 服务器验证所有输入消息的格式和内容
4. **连接限制**: 同一用户的新连接会替换旧连接
5. **错误不泄露**: 错误消息不包含敏感的服务器信息

## 性能考虑

1. **异步处理**: 使用独立线程处理 WebSocket 事件
2. **内存管理**: 及时清理断开的连接和空房间
3. **广播优化**: 批量发送消息减少系统调用
4. **数据库优化**: 异步保存消息避免阻塞实时通信

## 故障排除

### 常见问题

1. **连接失败**: 检查服务器是否启动，端口是否正确
2. **认证失败**: 验证 JWT 令牌是否有效，密钥是否正确
3. **消息丢失**: 检查网络连接，确认用户已加入房间
4. **重复消息**: 检查客户端是否有重复发送逻辑

### 调试工具

- 服务器日志：查看详细的连接和消息处理日志
- WebSocket 调试工具：使用浏览器开发者工具或专用工具
- 数据库查询：验证消息是否正确保存

---

**文档版本**: 1.0  
**最后更新**: 2025年7月20日  
**维护者**: SwiftChat 开发团队
