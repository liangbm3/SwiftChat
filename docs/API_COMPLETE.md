# SwiftChat API 文档

**版本**: 1.0.0  
**基础URL**: `http://localhost:8080`  
**WebSocket URL**: `ws://localhost:8081`

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

**响应**:
```json
{
  "success": true,
  "message": "Joined room successfully",
  "data": {
    "type": "room_joined",
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

**响应**:
```json
{
  "success": true,
  "message": "Left room successfully",
  "data": {
    "type": "room_left"
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

**广播响应**:
```json
{
  "success": true,
  "message": "Message sent successfully",
  "data": {
    "type": "message_received",
    "content": "Hello, everyone!",
    "user_id": "user_a3a80b0b",
    "username": "john_doe",
    "room_id": "room_12345",
    "timestamp": "2025-07-20T13:30:00Z"
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
  "message": "Pong",
  "data": {
    "type": "pong"
  }
}
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

---

**最后更新**: 2025-07-20  
**文档版本**: 1.0.0
