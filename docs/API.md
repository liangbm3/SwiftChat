# SwiftChat API 文档

http 端口：8080  
WebSocket 端口：8081

## 目录

1. [认证系统 (Authentication)](#认证系统)
2. [用户管理 (User Management)](#用户管理)
3. [房间管理 (Room Management)](#房间管理)
4. [消息管理 (Message Management)](#消息管理)
5. [系统API (System APIs)](#系统api)
6. [WebSocket API](#websocket-api)
7. [错误码说明](#错误码说明)
8. [响应格式说明](#响应格式说明)

---

## 认证系统

### 注册用户
**POST** `/api/v1/auth/register`

创建新用户账户。

**请求体**:
```json
{
  "username": "string",
  "password": "string"
}
```

**响应** (201 Created):
```json
{
  "success": true,
  "message": "User registered successfully",
  "data": {
    "token": "eyJhbGciOiJIUzI1NiIs...",
    "id": "user_a3a80b0b",
    "username": "example_user"
  }
}
```

**错误响应** (409 Conflict):
```json
{
  "success": false,
  "message": "User already exists",
  "error": "Username already taken"
}
```

### 用户登录
**POST** `/api/v1/auth/login`

用户身份验证并获取访问令牌。

**请求体**:
```json
{
  "username": "string",
  "password": "string"
}
```

**响应** (200 OK):
```json
{
  "success": true,
  "message": "Login successful",
  "data": {
    "token": "eyJhbGciOiJIUzI1NiIs...",
    "id": "user_a3a80b0b",
    "username": "example_user"
  }
}
```

**错误响应** (401 Unauthorized):
```json
{
  "success": false,
  "message": "Invalid credentials",
  "error": "Username or password is incorrect"
}
```

---

## 用户管理

### 获取当前用户信息
**GET** `/api/v1/users/me`

🔒 **需要认证**: Bearer Token

获取当前登录用户的详细信息。

**请求头**:
```
Authorization: Bearer <token>
```

**响应** (200 OK):
```json
{
  "success": true,
  "message": "User information retrieved successfully",
  "data": {
    "user": {
      "id": "user_a3a80b0b",
      "username": "example_user",
      "created_at": "2025-07-20T13:30:00Z"
    }
  }
}
```

### 获取用户列表
**GET** `/api/v1/users`

🔒 **需要认证**: Bearer Token

获取系统中所有用户的列表（支持分页）。

**查询参数**:
- `limit` (可选): 每页返回的用户数量，默认为10
- `offset` (可选): 跳过的用户数量，默认为0

**响应** (200 OK):
```json
{
  "success": true,
  "message": "Users retrieved successfully",
  "data": {
    "users": [
      {
        "id": "user_a3a80b0b",
        "username": "user1",
        "created_at": "2025-07-20T13:30:00Z"
      }
    ],
    "total": 25,
    "limit": 10,
    "offset": 0
  }
}
```

### 获取指定用户信息
**GET** `/api/v1/users/{user_id}`

🔒 **需要认证**: Bearer Token

获取指定用户的详细信息。

**路径参数**:
- `user_id`: 用户ID

**响应** (200 OK):
```json
{
  "success": true,
  "message": "User information retrieved successfully",
  "data": {
    "user": {
      "id": "user_a3a80b0b",
      "username": "example_user",
      "created_at": "2025-07-20T13:30:00Z"
    }
  }
}
```

---

## 房间管理

### 创建房间
**POST** `/api/v1/rooms`

🔒 **需要认证**: Bearer Token

创建新的聊天房间。

**请求体**:
```json
{
  "name": "string",
  "description": "string"
}
```

**响应** (201 Created):
```json
{
  "success": true,
  "message": "Room created successfully",
  "data": {
    "id": "room_12345",
    "name": "My Room",
    "description": "A test room",
    "creator_id": "user_a3a80b0b",
    "created_at": "2025-07-20T13:30:00Z"
  }
}
```

### 获取房间列表
**GET** `/api/v1/rooms`

获取所有可用房间的列表，支持分页查询。

**查询参数**:
- `limit` (可选): 返回房间数量限制，默认为50，最大为100
- `offset` (可选): 偏移量，默认为0

**响应** (200 OK):
```json
{
  "success": true,
  "message": "Successfully retrieved rooms",
  "data": {
    "rooms": [
      {
        "id": "room_12345",
        "name": "General Chat",
        "description": "Main discussion room",
        "creator_id": "user_a3a80b0b",
        "member_count": 5,
        "created_at": "2025-07-20T13:30:00Z"
      }
    ],
    "count": 1,
    "total": 25,
    "limit": 50,
    "offset": 0
  }
}
```

### 加入房间
**POST** `/api/v1/rooms/join`

🔒 **需要认证**: Bearer Token

加入指定的聊天房间。

**请求体**:
```json
{
  "room_id": "room_12345"
}
```

**响应** (200 OK):
```json
{
  "success": true,
  "message": "Successfully joined room",
  "data": {
    "room_id": "room_12345",
    "user_id": "user_a3a80b0b",
    "joined_at": "2025-07-20T13:30:00Z"
  }
}
```

### 离开房间
**POST** `/api/v1/rooms/leave`

🔒 **需要认证**: Bearer Token

离开指定的聊天房间。

**请求体**:
```json
{
  "room_id": "room_12345"
}
```

**响应** (200 OK):
```json
{
  "success": true,
  "message": "Successfully left room",
  "data": {
    "room_id": "room_12345",
    "user_id": "user_a3a80b0b"
  }
}
```

### 更新房间描述
**PATCH** `/api/v1/rooms/{room_id}`

🔒 **需要认证**: Bearer Token (仅房间创建者)

更新房间的描述信息。

**路径参数**:
- `room_id` (必需): 房间ID

**请求体**:
```json
{
  "description": "Updated description"
}
```

**响应** (200 OK):
```json
{
  "success": true,
  "message": "Room description updated successfully",
  "data": {
    "room_id": "room_12345",
    "new_description": "Updated description",
    "updated_by": "user_a3a80b0b"
  }
}
```

### 删除房间
**DELETE** `/api/v1/rooms/{room_id}`

🔒 **需要认证**: Bearer Token (仅房间创建者)

删除指定的聊天房间。

**路径参数**:
- `room_id` (必需): 房间ID

**响应** (200 OK):
```json
{
  "success": true,
  "message": "Room deleted successfully",
  "data": {
    "room_id": "room_12345",
    "deleted_by": "user_a3a80b0b"
  }
}
```

---

## 消息管理

### 获取房间消息
**GET** `/api/v1/messages`

🔒 **需要认证**: Bearer Token

获取指定房间的消息历史。

**查询参数**:
- `room_id` (必需): 房间ID
- `limit` (可选): 消息数量限制，默认为50，最大为100

**响应** (200 OK):
```json
{
  "success": true,
  "message": "Messages retrieved successfully",
  "data": {
    "messages": [
      {
        "id": 123,
        "room_id": "room_12345",
        "user_id": "user_a3a80b0b",
        "content": "Hello, world!",
        "timestamp": 1642694400,
        "sender": {
          "id": "user_a3a80b0b",
          "username": "john_doe",
          "password": "",
          "is_online": false
        }
      }
    ],
    "room_id": "room_12345",
    "count": 1
  }
}
```

---

## 系统API

### 健康检查
**GET** `/api/v1/health`

检查服务器运行状态。

**响应** (200 OK):
```json
{
  "success": true,
  "message": "Server is running",
  "data": {
    "status": "ok",
    "timestamp": 1753018736,
    "uptime": "unknown"
  }
}
```

### 服务器信息
**GET** `/api/v1/info`

获取服务器版本和配置信息。

**响应** (200 OK):
```json
{
  "success": true,
  "message": "Server information retrieved successfully",
  "data": {
    "name": "SwiftChat HTTP Server",
    "version": "1.0.0",
    "description": "A simple HTTP server with WebSocket support",
    "timestamp": 1753018736
  }
}
```

### Echo 测试 (GET)
**GET** `/api/v1/echo`

回显请求信息，用于测试连接。

**响应** (200 OK):
```json
{
  "success": true,
  "message": "Echo GET request received",
  "data": {
    "method": "GET",
    "path": "/api/v1/echo",
    "user_agent": "curl/7.68.0",
    "timestamp": 1753018746
  }
}
```

### Echo 测试 (POST)
**POST** `/api/v1/echo`

回显请求信息和请求体数据。

**响应** (200 OK):
```json
{
  "success": true,
  "message": "Echo POST request received",
  "data": {
    "method": "POST",
    "path": "/api/v1/echo",
    "received_data": "{\"test\": \"data\"}",
    "timestamp": 1753018746
  }
}
```

### 受保护端点示例
**GET** `/api/v1/protected`

🔒 **需要认证**: Bearer Token

演示认证中间件的示例端点。

**响应** (200 OK):
```json
{
  "success": true,
  "message": "This is a protected endpoint",
  "data": {
    "secret_info": "Secret information",
    "timestamp": 1753018746,
    "access_level": "authenticated"
  }
}
```

---

## WebSocket API

### 连接
**URL**: `ws://localhost:8081`

### 消息格式

所有WebSocket消息都使用统一的JSON格式：

**客户端发送格式**:
```json
{
  "type": "message_type",
  "data": {
    // 具体数据
  }
}
```

**服务器响应格式**:
```json
{
  "success": true,
  "message": "描述信息",
  "data": {
    "type": "response_type",
    // 其他数据
  }
}
```

### 支持的消息类型

#### 1. 用户认证
**发送**:
```json
{
  "type": "auth",
  "token": "eyJhbGciOiJIUzI1NiIs..."
}
```

**响应**:
```json
{
  "success": true,
  "message": "WebSocket authentication successful",
  "data": {
    "user_id": "user_a3a80b0b",
    "status": "connected"
  }
}
```

#### 2. 加入房间
**发送**:
```json
{
  "type": "join_room",
  "room_id": "room_12345"
}
```

**成功响应（发送给发起者）**:
```json
{
  "success": true,
  "message": "Room joined successfully",
  "data": {
    "type": "room_joined",
    "room_id": "room_12345",
    "user_id": "user_a3a80b0b"
  }
}
```

**广播通知（发送给房间内其他用户）**:
```json
{
  "success": true,
  "message": "User joined room",
  "data": {
    "type": "user_joined",
    "user_id": "user_a3a80b0b",
    "username": "john_doe",
    "room_id": "room_12345"
  }
}
```

#### 3. 离开房间
**发送**:
```json
{
  "type": "leave_room"
}
```

**成功响应（发送给发起者）**:
```json
{
  "success": true,
  "message": "Room left successfully",
  "data": {
    "type": "room_left",
    "room_id": "room_12345",
    "user_id": "user_a3a80b0b"
  }
}
```

**广播通知（发送给房间内剩余用户）**:
```json
{
  "success": true,
  "message": "User left room",
  "data": {
    "type": "user_left",
    "user_id": "user_a3a80b0b",
    "username": "john_doe",
    "room_id": "room_12345"
  }
}
```

#### 4. 发送消息
**发送**:
```json
{
  "type": "send_message",
  "content": "Hello, everyone!"
}
```

**成功响应（发送给发送者）**:
```json
{
  "success": true,
  "message": "Message sent successfully",
  "data": {
    "type": "message_sent",
    "user_id": "user_a3a80b0b",
    "username": "john_doe",
    "room_id": "room_12345",
    "content": "Hello, everyone!",
    "timestamp": 1721554800
  }
}
```

**广播响应（发送给房间内其他用户）**:
```json
{
  "success": true,
  "message": "Message sent successfully",
  "data": {
    "type": "message_received",
    "user_id": "user_a3a80b0b",
    "username": "john_doe",
    "room_id": "room_12345",
    "content": "Hello, everyone!",
    "timestamp": 1721554800
  }
}
```

#### 5. 心跳检测
**发送**:
```json
{
  "type": "ping"
}
```

**响应**:
```json
{
  "success": true,
  "message": "Pong response",
  "data": {
    "type": "pong",
    "timestamp": 1721554800
  }
}
```

### 被动接收的广播通知

除了主动发送消息的响应外，客户端还可能接收到以下类型的广播通知：

#### 1. 用户加入房间通知
当其他用户加入当前房间时收到：
```json
{
  "success": true,
  "message": "User joined room",
  "data": {
    "type": "user_joined",
    "user_id": "user_b4b91c1c",
    "username": "jane_doe",
    "room_id": "room_12345"
  }
}
```

#### 2. 用户离开房间通知
当其他用户离开当前房间时收到：
```json
{
  "success": true,
  "message": "User left room",
  "data": {
    "type": "user_left",
    "user_id": "user_b4b91c1c",
    "username": "jane_doe",
    "room_id": "room_12345"
  }
}
```

#### 3. 新消息通知
当房间内有新消息时收到：
```json
{
  "success": true,
  "message": "Message sent successfully",
  "data": {
    "type": "message_received",
    "user_id": "user_b4b91c1c",
    "username": "jane_doe",
    "room_id": "room_12345",
    "content": "Hello, everyone!",
    "timestamp": 1721554800
  }
}
```

### WebSocket 连接特性与注意事项

#### 连接生命周期
1. **建立连接**: 客户端连接到 `ws://localhost:8081`
2. **身份认证**: 连接后必须立即发送认证消息，否则连接将被断开
3. **消息处理**: 认证成功后可以发送业务消息
4. **连接断开**: 用户断开连接时会自动离开当前房间并通知其他用户

#### 重要特性
- **自动重连处理**: 如果用户已有活跃连接，新连接会自动关闭旧连接
- **房间自动切换**: 用户加入新房间时会自动离开当前房间
- **消息持久化**: 聊天消息会自动保存到数据库
- **线程安全**: 服务器使用互斥锁确保并发安全

#### 错误处理机制
- **认证超时**: 连接建立后30秒内未认证将被断开
- **无效消息**: 发送无效JSON或未知消息类型会收到错误响应
- **权限检查**: 发送消息需要先加入房间
- **数据库错误**: 消息保存失败会返回错误但不影响广播

### WebSocket 错误处理

**认证失败**:
```json
{
  "success": false,
  "message": "Authentication failed",
  "error": "Invalid or expired token"
}
```

**请求格式错误**:
```json
{
  "success": false,
  "message": "Request failed",
  "error": "Invalid JSON format"
}
```

**业务逻辑错误**:
```json
{
  "success": false,
  "message": "Request failed",
  "error": "You are not in any room"
}
```

**未知消息类型**:
```json
{
  "success": false,
  "message": "Request failed",
  "error": "Unknown message type: invalid_type"
}
```

**数据库错误**:
```json
{
  "success": false,
  "message": "Failed to save message",
  "error": "Database operation failed"
}
```

### WebSocket 使用示例

#### JavaScript 客户端示例
```javascript
// 连接WebSocket
const ws = new WebSocket('ws://localhost:8081');

// 连接打开时进行认证
ws.onopen = function() {
    console.log('WebSocket连接已建立');
    // 发送认证消息
    ws.send(JSON.stringify({
        type: 'auth',
        token: 'your_jwt_token_here'
    }));
};

// 处理接收到的消息
ws.onmessage = function(event) {
    const message = JSON.parse(event.data);
    console.log('收到消息:', message);
    
    if (message.success && message.data) {
        switch(message.data.type) {
            case 'room_joined':
                console.log('成功加入房间:', message.data.room_id);
                break;
            case 'user_joined':
                console.log('用户加入房间:', message.data.user_id);
                break;
            case 'message_received':
                console.log('新消息:', message.data.content);
                break;
            case 'pong':
                console.log('心跳响应');
                break;
        }
    }
};

// 发送消息的函数
function sendMessage(content) {
    if (ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({
            type: 'send_message',
            content: content
        }));
    }
}

// 加入房间
function joinRoom(roomId) {
    if (ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({
            type: 'join_room',
            room_id: roomId
        }));
    }
}

// 心跳检测
setInterval(() => {
    if (ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({
            type: 'ping'
        }));
    }
}, 30000); // 每30秒发送一次心跳
```

---

## 错误码说明

| HTTP状态码 | 说明 |
|-----------|------|
| 200 | OK - 请求成功 |
| 201 | Created - 资源创建成功 |
| 400 | Bad Request - 请求格式错误 |
| 401 | Unauthorized - 认证失败或缺少认证信息 |
| 403 | Forbidden - 权限不足 |
| 404 | Not Found - 资源不存在 |
| 409 | Conflict - 资源冲突（如用户名已存在） |
| 500 | Internal Server Error - 服务器内部错误 |

---

## 响应格式说明

### 成功响应格式
```json
{
  "success": true,
  "message": "操作成功的描述信息",
  "data": {
    // 具体的响应数据
  }
}
```

### 错误响应格式
```json
{
  "success": false,
  "message": "错误的描述信息",
  "error": "详细的错误信息"
}
```

### 认证
大部分API端点需要Bearer Token认证：

```http
Authorization: Bearer <your_jwt_token>
```

JWT Token通过登录或注册接口获取，有效期为1小时。

### CORS
服务器已配置CORS支持，允许跨域请求：
- `Access-Control-Allow-Origin: *`
- `Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS`
- `Access-Control-Allow-Headers: Content-Type, Authorization, X-Requested-With`

