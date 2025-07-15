# 数据库管理器 (DatabaseManager) API 文档

## 1\. 概述 (Overview)

`DatabaseManager` 是数据库访问层的核心入口，它遵循**外观模式 (Facade Pattern)**，为上层业务逻辑提供了一个统一、简洁且线程安全的接口来与数据库进行交互。

所有数据库操作都应通过实例化该类来完成。它内部管理着数据库连接以及各个数据实体的仓库（Repository），调用者无需关心底层实现细节。

**核心约定:**

  * **ID 优先**: 所有核心操作（如修改、删除、添加关联）都应使用实体的唯一ID (`user_id`, `room_id`)。
  * **`std::optional` 返回值**: 所有查找单个实体的方法（如 `getUserById`）均返回 `std::optional<T>`。调用者必须先检查其是否有值 (`.has_value()`)，再获取其中的数据 (`.value()` 或 `*`)，这是一种更安全的API设计。

### 实例化

```cpp
#include "db/database_manager.hpp"

// 在服务启动时创建实例
auto db_manager = std::make_unique<DatabaseManager>("/path/to/your/chat.db");

if (!db_manager->isConnected()) {
    // 处理数据库连接失败的错误
}
```

## 2\. API 详解

### 2.1 核心方法 (Core Methods)

-----

#### `DatabaseManager(const std::string &db_path)`

  * **描述**: 构造函数。创建一个 `DatabaseManager` 实例，并初始化数据库连接、创建所有必要的数据表和仓库。
  * **参数**:
      * `db_path` (`const std::string&`): SQLite 数据库文件的路径。
  * **返回值**: 无。

-----

#### `bool isConnected() const`

  * **描述**: 检查数据库是否已成功连接并初始化。
  * **参数**: 无。
  * **返回值**:
      * `true`: 连接成功。
      * `false`: 连接失败。

-----

### 2.2 用户操作 (User Operations)

-----

#### `bool createUser(const std::string &username, const std::string &password_hash)`

  * **描述**: 创建一个新用户。用户名具有唯一性约束。
  * **参数**:
      * `username` (`const std::string&`): 用户的显示名称（必须唯一）。
      * `password_hash` (`const std::string&`): 经过哈希处理的密码。
  * **返回值**: `bool` - `true` 表示创建成功，`false` 表示失败（如用户名已存在）。

-----

#### `bool validateUser(const std::string &username, const std::string &password_hash)`

  * **描述**: 验证用户名和密码哈希是否匹配。
  * **参数**:
      * `username` (`const std::string&`): 用户名。
      * `password_hash` (`const std::string&`): 密码哈希。
  * **返回值**: `bool` - `true` 表示验证通过，`false` 表示失败。

-----

#### `bool userExists(const std::string &user_id)`

  * **描述**: 根据用户ID检查用户是否存在。
  * **参数**:
      * `user_id` (`const std::string&`): 用户的唯一ID。
  * **返回值**: `bool` - `true` 表示存在，`false` 表示不存在。

-----

#### `std::optional<User> getUserById(const std::string &user_id) const`

  * **描述**: 根据用户ID查找并返回一个完整的`User`对象。
  * **参数**:
      * `user_id` (`const std::string&`): 用户的唯一ID。
  * **返回值**: `std::optional<User>` - 如果找到，返回包含`User`对象的`optional`；否则返回`std::nullopt`。

-----

#### `std::optional<User> getUserByUsername(const std::string &username) const`

  * **描述**: 根据用户名查找并返回一个完整的`User`对象。这是将用户输入（用户名）转换成系统内部ID的关键方法。
  * **参数**:
      * `username` (`const std::string&`): 用户名。
  * **返回值**: `std::optional<User>` - 如果找到，返回包含`User`对象的`optional`；否则返回`std::nullopt`。

-----

*其他用户操作方法...* (`setUserOnlineStatus`, `getAllUsers`, etc.)

-----

### 2.3 房间操作 (Room Operations)

-----

#### `std::optional<nlohmann::json> createRoom(const std::string &name, const std::string &creator_id)`

  * **描述**: 创建一个新的聊天室。房间名具有唯一性约束。
  * **参数**:
      * `name` (`const std::string&`): 聊天室的显示名称（必须唯一）。
      * `creator_id` (`const std::string&`): 创建者的用户ID。
  * **返回值**: `std::optional<nlohmann::json>` - 如果创建成功，返回包含新房间完整信息（包括生成的`id`）的`json`对象；如果失败（如房间名已存在），则返回`std::nullopt`。

-----

#### `bool deleteRoom(const std::string &room_id)`

  * **描述**: 根据房间ID删除一个聊天室。
  * **参数**:
      * `room_id` (`const std::string&`): 房间的唯一ID。
  * **返回值**: `bool` - `true` 表示删除成功。

-----

#### `std::optional<nlohmann::json> getRoomById(const std::string &room_id) const`

  * **描述**: 根据房间ID查找并返回聊天室的详细信息。
  * **参数**:
      * `room_id` (`const std::string&`): 房间的唯一ID。
  * **返回值**: `std::optional<nlohmann::json>` - 如果找到，返回包含房间信息的`json`对象；否则返回`std::nullopt`。返回的`json`结构示例：`{"id": "...", "name": "...", "creator_id": "...", ...}`

-----

*其他房间操作方法...* (`updateRoom`, `getRooms`, etc.)

-----

### 2.4 房间成员操作 (Room Member Operations)

-----

#### `bool addRoomMember(const std::string &room_id, const std::string &user_id)`

  * **描述**: 将一个用户添加为聊天室的成员。
  * **参数**:
      * `room_id` (`const std::string&`): 房间的唯一ID。
      * `user_id` (`const std::string&`): 用户的唯一ID。
  * **返回值**: `bool` - `true` 表示添加成功。

-----

#### `std::vector<nlohmann::json> getRoomMembers(const std::string &room_id) const`

  * **描述**: 获取一个聊天室的所有成员列表。
  * **参数**:
      * `room_id` (`const std::string&`): 房间的唯一ID。
  * **返回值**: `std::vector<nlohmann::json>` - 包含所有成员信息的`json`对象列表。如果房间没有成员，返回空`vector`。`json`结构示例：`{"id": "...", "username": "...", "is_online": ..., ...}`

-----

*其他成员操作方法...* (`removeRoomMember`)

-----

### 2.5 消息操作 (Message Operations)

-----

#### `bool saveMessage(const std::string &room_id, const std::string &user_id, ...)`

  * **描述**: 保存一条聊天消息到指定的房间。
  * **参数**:
      * `room_id` (`const std::string&`): 房间的唯一ID。
      * `user_id` (`const std::string&`): 发送者的用户ID。
      * `content` (`const std::string&`): 消息内容。
      * `timestamp` (`int64_t`): 消息时间戳。
  * **返回值**: `bool` - `true` 表示保存成功。

-----

#### `std::vector<nlohmann::json> getMessages(const std::string &room_id, int limit, ...)`

  * **描述**: 获取指定房间的历史消息。
  * **参数**:
      * `room_id` (`const std::string&`): 房间的唯一ID。
      * `limit` (`int`): 获取消息的最大数量。
      * `before_timestamp` (`int64_t`): 获取此时间戳之前的消息（用于分页加载）。
  * **返回值**: `std::vector<nlohmann::json>` - 包含消息的`json`对象列表。`json`结构包含一个嵌套的`sender`对象：`{"id": ..., "content": "...", "sender": {"id": "...", "username": "..."}}`

-----

### 3\. 使用示例 (Usage Example)

这是一个典型的业务逻辑流程，展示了如何协同使用多个API。

```cpp
void handleClientRequest(DatabaseManager& db_manager, const std::string& admin_name, const std::string& guest_name) {
    // 步骤 1: 将外部输入（用户名）转换成内部ID
    auto admin_opt = db_manager.getUserByUsername(admin_name);
    auto guest_opt = db_manager.getUserByUsername(guest_name);

    if (!admin_opt || !guest_opt) {
        std::cerr << "管理员或访客不存在!" << std::endl;
        return;
    }
    
    User admin = *admin_opt;
    User guest = *guest_opt;

    // 步骤 2: 使用ID创建新房间，并从返回值中获取新房间的ID
    auto room_opt = db_manager.createRoom("新项目讨论组", admin.getId());
    if (!room_opt) {
        std::cerr << "创建房间失败!" << std::endl;
        return;
    }
    std::string room_id = room_opt->at("id").get<std::string>();

    // 步骤 3: 使用ID添加成员
    db_manager.addRoomMember(room_id, admin.getId()); // 把自己也加进去
    db_manager.addRoomMember(room_id, guest.getId());

    // 步骤 4: 使用ID发送消息
    int64_t now = std::chrono::system_clock::now().time_since_epoch().count();
    db_manager.saveMessage(room_id, guest.getId(), "大家好，我是新成员！", now);

    std::cout << "流程成功完成!" << std::endl;
}
```