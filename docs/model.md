# 模型类设计文档

模型类的实现位于 `src/model/` 目录下，用于表示系统中的核心数据实体。

## 1. 概述

SwiftChat 包含三个主要的模型类：
- `User` - 用户实体
- `Room` - 聊天房间实体  
- `Message` - 消息实体

所有模型类都支持 JSON 序列化和反序列化，使用 nlohmann/json 库。

## 2. User 类

`User` 类表示系统中的用户实体，包含用户的基本信息如ID、用户名和密码。

### 2.1 成员变量

| 变量名 | 类型 | 描述 |
|--------|------|------|
| `id_` | `std::string` | 用户唯一标识符 |
| `username_` | `std::string` | 用户名 |
| `password_` | `std::string` | 用户密码 |

### 2.2 构造函数

```cpp
User()  // 默认构造函数
User(const std::string &id, const std::string &username, const std::string &password)
```

### 2.3 公共方法

#### Getter 方法

- `const std::string& getId() const` - 获取用户ID
- `const std::string& getUsername() const` - 获取用户名
- `const std::string& getPassword() const` - 获取密码

#### Setter 方法

- `void setId(const std::string &id)` - 设置用户ID
- `void setUsername(const std::string &username)` - 设置用户名
- `void setPassword(const std::string &password)` - 设置密码

#### JSON 转换方法

- `json toJson() const` - 将用户对象转换为JSON格式
- `static User fromJson(const json &j)` - 从JSON创建用户对象

### 2.4 JSON 格式

```json
{
    "id": "user_id",
    "username": "user_name", 
    "password": "user_password"
}
```

## 3. Room 类

`Room` 类表示聊天房间实体，包含房间的基本信息如ID、名称、描述、创建者和创建时间。

### 3.1 成员变量

| 变量名 | 类型 | 描述 |
|--------|------|------|
| `id_` | `std::string` | 房间唯一标识符 |
| `name_` | `std::string` | 房间名称 |
| `description_` | `std::string` | 房间描述 |
| `creator_id_` | `std::string` | 创建者用户ID |
| `created_at_` | `int64_t` | 创建时间戳 |

### 3.2 构造函数

```cpp
Room()  // 默认构造函数，created_at_ 初始化为 0
Room(const std::string &id, const std::string &name, const std::string &description,
     const std::string &creator_id, int64_t created_at)
```

### 3.3 公共方法

#### Getter 方法

- `const std::string& getId() const` - 获取房间ID
- `const std::string& getName() const` - 获取房间名称
- `const std::string& getDescription() const` - 获取房间描述
- `const std::string& getCreatorId() const` - 获取创建者ID
- `int64_t getCreatedAt() const` - 获取创建时间戳

#### Setter 方法

- `void setId(const std::string &id)` - 设置房间ID
- `void setName(const std::string &name)` - 设置房间名称
- `void setDescription(const std::string &description)` - 设置房间描述
- `void setCreatorId(const std::string &creator_id)` - 设置创建者ID
- `void setCreatedAt(int64_t created_at)` - 设置创建时间戳

#### JSON 转换方法

- `json toJson() const` - 将房间对象转换为JSON格式
- `static Room fromJson(const json &j)` - 从JSON创建房间对象

### 3.4 JSON 格式

```json
{
    "id": "room_id",
    "name": "room_name",
    "description": "room_description",
    "creator_id": "creator_user_id",
    "created_at": 1642694400
}
```

---

## 4. Message 类

`Message` 类表示聊天消息实体，包含消息的基本信息和发送者用户名。

### 4.1 成员变量

| 变量名 | 类型 | 描述 |
|--------|------|------|
| `id_` | `int64_t` | 消息唯一标识符 |
| `room_id_` | `std::string` | 所属房间ID |
| `user_id_` | `std::string` | 发送者用户ID |
| `content_` | `std::string` | 消息内容 |
| `timestamp_` | `int64_t` | 发送时间戳 |
| `user_name_` | `std::string` | 发送者用户名 |

### 4.2 构造函数

```cpp
Message()  // 默认构造函数，id_ 和 timestamp_ 初始化为 0

Message(int64_t id, const std::string &room_id, const std::string &user_id,
        const std::string &content, int64_t timestamp, const std::string &user_name)
```

### 4.3 公共方法

#### Getter 方法

- `int64_t getId() const` - 获取消息ID
- `const std::string& getRoomId() const` - 获取房间ID
- `const std::string& getUserId() const` - 获取发送者用户ID
- `const std::string& getContent() const` - 获取消息内容
- `int64_t getTimestamp() const` - 获取时间戳
- `const std::string& getUserName() const` - 获取发送者用户名

#### Setter 方法

- `void setId(int64_t id)` - 设置消息ID
- `void setRoomId(const std::string &room_id)` - 设置房间ID
- `void setUserId(const std::string &user_id)` - 设置发送者用户ID
- `void setContent(const std::string &content)` - 设置消息内容
- `void setTimestamp(int64_t timestamp)` - 设置时间戳
- `void setUserName(const std::string &user_name)` - 设置发送者用户名

#### JSON 转换方法

- `json toJson() const` - 将消息对象转换为JSON格式
- `static Message fromJson(const json &j)` - 从JSON创建消息对象

### 4.4 JSON 格式

```json
{
    "id": 12345,
    "room_id": "room_id",
    "user_id": "sender_user_id", 
    "content": "Hello, World!",
    "timestamp": 1642694400,
    "user_name": "sender_name"
}
```

