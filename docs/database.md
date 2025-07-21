# 数据层设计文档

## 1\. 概述

数据层的实现位于`src/db/`目录下，数据层的作用是为上层提供操作数据库的接口，对数据进行持久化存储。数据层使用 SQLite 3 数据库，并且采用仓库模式和依赖注入等设计模式，架构清晰，易于拓展。

所有数据库操作都通过 `std::recursive_mutex` 加锁，确保在多线程环境下的数据一致性和安全性。并且所有数据插入和查询都使用了 `sqlite3_prepare_v2` 和 `sqlite3_bind_*` 系列函数，有效防止了 SQL 注入攻击。为频繁查询的字段（如 `username` 和 `room_name`）建立了索引，以提高查询性能。数据表设置了级联删除，有效防止孤儿数据的出现：
- rooms 表
    - 当用户被删除时，该用户创建的所有房间也会被自动删除
- room_members 表
    - 当房间被删除时，该房间的所有成员关系记录会被自动删除
    - 当用户被删除时，该用户在所有房间的成员关系记录会被自动删除
- messages 表
    - 当房间被删除时，该房间的所有消息会被自动删除
    - 当用户被删除时，该用户发送的所有消息会被自动删除

## 2\. 数据库表结构

数据库包含以下四个核心表：

### 2.1. `users` 表

存储用户的基本信息和状态。

| 字段名 (Column) | 数据类型 (Type) | 约束 (Constraints) | 描述 (Description) |
| :--- | :--- | :--- | :--- |
| `id` | `TEXT` | `PRIMARY KEY` | 用户的唯一标识符 (例如: `user_a1b2c3d4`)。 |
| `username` | `TEXT` | `UNIQUE NOT NULL` | 用户的显示名称，必须唯一。 |
| `password_hash` | `TEXT` | `NOT NULL` | 存储用户密码的哈希值。 |
| `created_at` | `INTEGER` | `NOT NULL` | 账户创建时间的 Unix 时间戳 (nanoseconds)。 |


### 2.2. `rooms` 表

存储聊天室的基本信息。

| 字段名 (Column) | 数据类型 (Type) | 约束 (Constraints) | 描述 (Description) |
| :--- | :--- | :--- | :--- |
| `id` | `TEXT` | `PRIMARY KEY` | 聊天室的唯一标识符 (例如: `room_x1y2z3w4`)。 |
| `name` | `TEXT` | `UNIQUE NOT NULL` | 聊天室的显示名称，必须唯一。 |
| `description` | `TEXT` | `DEFAULT ''` | 聊天室的描述信息。 |
| `creator_id` | `TEXT` | `NOT NULL, FOREIGN KEY` | 创建该聊天室的用户ID。外键，关联 `users(id)`。 |
| `created_at` | `INTEGER` | `NOT NULL` | 聊天室创建时间的 Unix 时间戳 (nanoseconds)。 |

### 2.3. `room_members` 表

这是一个**连接表 (Junction Table)**，用于表示用户和聊天室之间的多对多关系。

| 字段名 (Column) | 数据类型 (Type) | 约束 (Constraints) | 描述 (Description) |
| :--- | :--- | :--- | :--- |
| `room_id` | `TEXT` | `PRIMARY KEY, FOREIGN KEY` | 聊天室的ID。外键，关联 `rooms(id)`。 |
| `user_id` | `TEXT` | `PRIMARY KEY, FOREIGN KEY` | 用户的ID。外键，关联 `users(id)`。 |
| `joined_at` | `INTEGER` | `NOT NULL` | 用户加入该聊天室的 Unix 时间戳 (nanoseconds)。 |

**说明**: `(room_id, user_id)` 组成一个复合主键，确保一个用户在一个聊天室里只有一条成员记录。

### 2.4. `messages` 表

存储所有聊天消息。

| 字段名 (Column) | 数据类型 (Type) | 约束 (Constraints) | 描述 (Description) |
| :--- | :--- | :--- | :--- |
| `id` | `INTEGER` | `PRIMARY KEY AUTOINCREMENT` | 每条消息的唯一自增ID。 |
| `room_id` | `TEXT` | `NOT NULL, FOREIGN KEY` | 消息所属聊天室的ID。外键，关联 `rooms(id)`。 |
| `user_id` | `TEXT` | `NOT NULL, FOREIGN KEY` | 消息发送者的用户ID。外键，关联 `users(id)`。 |
| `content` | `TEXT` | `NOT NULL` | 消息的文本内容。 |
| `timestamp` | `INTEGER` | `NOT NULL` | 消息发送的 Unix 时间戳 (nanoseconds)。 |


## 3\. 数据库 API

`DatabaseManager` 是数据库访问层的核心入口，它遵循**外观模式 (Facade Pattern)**，为上层业务逻辑提供了一个统一、简洁且线程安全的接口来与数据库进行交互。

所有数据库操作都应通过实例化该类来完成。它内部管理着数据库连接以及各个数据实体的仓库（Repository），调用者无需关心底层实现细节。

**核心约定:**
-  **ID 优先**: 所有核心操作（如修改、删除、添加关联）都应使用实体的唯一ID (`user_id`, `room_id`)。
-  **`std::optional` 返回值**: 所有查找单个实体的方法（如 `getUserById`）均返回 `std::optional<T>`。调用者必须先检查其是否有值 (`.has_value()`)，再获取其中的数据 (`.value()` 或 `*`)，这是一种更安全的API设计。

### 3.1 核心方法

---

#### `DatabaseManager(const std::string &db_path)`

- **描述**: 构造函数。创建一个 `DatabaseManager` 实例，并初始化数据库连接、创建所有必要的数据表和仓库。
- **参数**:
    - `db_path` (`const std::string&`): SQLite 数据库文件的路径。
- **返回值**: 无。

---

#### `bool isConnected() const`

- **描述**: 检查数据库是否已成功连接并初始化。
- **参数**: 无。
- **返回值**:
    - `true`: 连接成功。
    - `false`: 连接失败。

---

### 3.2 用户操作

---

#### `bool createUser(const std::string &username, const std::string &password_hash)`

-  **描述**: 创建一个新用户。用户名具有唯一性约束。
- **参数**:
    - `username` (`const std::string&`): 用户的显示名称（必须唯一）。
    - `password_hash` (`const std::string&`): 经过哈希处理的密码。
- **返回值**: `bool` - `true` 表示创建成功，`false` 表示失败（如用户名已存在）。

---

#### `bool validateUser(const std::string &username, const std::string &password_hash)`

- **描述**: 验证用户名和密码哈希是否匹配。
- **参数**:
    - `username` (`const std::string&`): 用户名。
    - `password_hash` (`const std::string&`): 密码哈希。
- **返回值**: `bool` - `true` 表示验证通过，`false` 表示失败。

---

#### `bool userExists(const std::string &user_id)`

- **描述**: 根据用户ID检查用户是否存在。
- **参数**:
    - `user_id` (`const std::string&`): 用户的唯一ID。
- **返回值**: `bool` - `true` 表示存在，`false` 表示不存在。

---

#### `std::optional<User> getUserById(const std::string &user_id) const`

- **描述**: 根据用户ID查找并返回一个完整的`User`对象。
- **参数**:
      - `user_id` (`const std::string&`): 用户的唯一ID。
- **返回值**: `std::optional<User>` - 如果找到，返回包含`User`对象的`optional`；否则返回`std::nullopt`。

---

#### `std::optional<User> getUserByUsername(const std::string &username) const`

- **描述**: 根据用户名查找并返回一个完整的`User`对象。这是将用户输入（用户名）转换成系统内部ID的关键方法。
- **参数**:
      - `username` (`const std::string&`): 用户名。
- **返回值**: `std::optional<User>` - 如果找到，返回包含`User`对象的`optional`；否则返回`std::nullopt`。

---

#### `std::vector<User> getAllUsers()`

- **描述**: 获取所有用户的详细信息。
- **参数**: 无。
- **返回值**: `std::vector<User>` - 包含所有用户完整信息的向量。

---

### 3.3 房间操作

#### 房间基本操作

---

#### `std::optional<Room> createRoom(const std::string &name, const std::string &description, const std::string &creator_id)`

- **描述**: 创建一个新的聊天室。房间名具有唯一性约束。
- **参数**:
      - `name` (`const std::string&`): 聊天室的显示名称（必须唯一）。
      - `description` (`const std::string&`): 聊天室的描述信息。
      - `creator_id` (`const std::string&`): 创建者的用户ID。
- **返回值**: `std::optional<Room>` - 如果创建成功，返回包含新房间完整信息的`Room`对象；如果失败（如房间名已存在），则返回`std::nullopt`。

---

#### `bool deleteRoom(const std::string &room_id)`

- **描述**: 根据房间ID删除一个聊天室。级联删除该房间的所有消息和成员关系。
- **参数**:
      - `room_id` (`const std::string&`): 房间的唯一ID。
- **返回值**: `bool` - `true` 表示删除成功，`false` 表示失败。

---

#### `bool roomExists(const std::string &room_id)`

- **描述**: 根据房间ID检查房间是否存在。
- **参数**:
      - `room_id` (`const std::string&`): 房间的唯一ID。
- **返回值**: `bool` - `true` 表示存在，`false` 表示不存在。

---

#### `bool updateRoom(const std::string &room_id, const std::string &name, const std::string &description)`

- **描述**: 更新房间的名称和描述信息。
- **参数**:
      - `room_id` (`const std::string&`): 房间的唯一ID。
      - `name` (`const std::string&`): 新的房间名称。
      - `description` (`const std::string&`): 新的房间描述。
- **返回值**: `bool` - `true` 表示更新成功，`false` 表示失败。

---

#### 房间查询

#### `std::vector<std::string> getRooms()`

- **描述**: 获取所有房间的名称列表。
- **参数**: 无。
- **返回值**: `std::vector<std::string>` - 包含所有房间名称的向量。

---

#### `std::vector<Room> getAllRooms()`

- **描述**: 获取所有房间的详细信息。
- **参数**: 无。
- **返回值**: `std::vector<Room>` - 包含所有房间完整信息的向量。

---

#### `std::optional<Room> getRoomById(const std::string &room_id) const`

- **描述**: 根据房间ID查找并返回一个完整的`Room`对象。
- **参数**:
      - `room_id` (`const std::string&`): 房间的唯一ID。
- **返回值**: `std::optional<Room>` - 如果找到，返回包含`Room`对象的`optional`；否则返回`std::nullopt`。

---

#### `std::optional<std::string> getRoomIdByName(const std::string &room_name) const`

- **描述**: 根据房间名查找并返回房间ID。这是将用户输入（房间名）转换成系统内部ID的关键方法。
- **参数**:
      - `room_name` (`const std::string&`): 房间名称。
- **返回值**: `std::optional<std::string>` - 如果找到，返回包含房间ID的`optional`；否则返回`std::nullopt`。

---

#### `bool isRoomCreator(const std::string &room_id, const std::string &user_id)`

- **描述**: 检查指定用户是否为指定房间的创建者。
- **参数**:
      - `room_id` (`const std::string&`): 房间的唯一ID。
      - `user_id` (`const std::string&`): 用户的唯一ID。
- **返回值**: `bool` - `true` 表示是创建者，`false` 表示不是。

---

#### `std::string generateRoomId()`

- **描述**: 生成一个唯一的房间ID。
- **参数**: 无。
- **返回值**: `std::string` - 新生成的房间ID。

---

#### 房间成员管理

#### `std::vector<nlohmann::json> getRoomMembers(const std::string &room_id) const`

- **描述**: 获取指定房间的所有成员信息。
- **参数**:
      - `room_id` (`const std::string&`): 房间的唯一ID。
- **返回值**: `std::vector<nlohmann::json>` - 包含房间所有成员信息的JSON对象向量。

---

#### `std::vector<Room> getUserJoinedRooms(const std::string &user_id) const`

- **描述**: 获取指定用户已加入的所有房间列表。
- **参数**:
      - `user_id` (`const std::string&`): 用户的唯一ID。
- **返回值**: `std::vector<Room>` - 包含用户已加入房间信息的向量。

---

#### `bool addRoomMember(const std::string &room_id, const std::string &user_id)`

- **描述**: 将指定用户添加到指定房间。
- **参数**:
      - `room_id` (`const std::string&`): 房间的唯一ID。
      - `user_id` (`const std::string&`): 用户的唯一ID。
- **返回值**: `bool` - `true` 表示添加成功，`false` 表示失败（如用户已在房间中）。

---

#### `bool removeRoomMember(const std::string &room_id, const std::string &user_id)`

- **描述**: 从指定房间移除指定用户。
- **参数**:
      - `room_id` (`const std::string&`): 房间的唯一ID。
      - `user_id` (`const std::string&`): 用户的唯一ID。
- **返回值**: `bool` - `true` 表示移除成功，`false` 表示失败。

### 3.4 消息操作

---

#### `bool saveMessage(const std::string &room_id, const std::string &user_id, const std::string &content, int64_t timestamp)`

- **描述**: 保存一条新消息到指定房间。
- **参数**:
      - `room_id` (`const std::string&`): 消息所属房间的ID。
      - `user_id` (`const std::string&`): 消息发送者的用户ID。
      - `content` (`const std::string&`): 消息内容。
      - `timestamp` (`int64_t`): 消息发送的时间戳（纳秒）。
- **返回值**: `bool` - `true` 表示保存成功，`false` 表示失败。

---

#### `std::vector<Message> getMessages(const std::string &room_id, int limit = 50, int64_t before_timestamp = 0)`

- **描述**: 获取指定房间的消息列表，支持分页和时间过滤。
- **参数**:
      - `room_id` (`const std::string&`): 房间的唯一ID。
      - `limit` (`int`): 返回消息的最大数量，默认50条。
      - `before_timestamp` (`int64_t`): 获取此时间戳之前的消息，0表示获取最新消息。
- **返回值**: `std::vector<Message>` - 包含消息信息的向量，按时间戳倒序排列。

---

#### `std::optional<Message> getMessageById(int64_t message_id)`

- **描述**: 根据消息ID获取单条消息的详细信息。
- **参数**:
      - `message_id` (`int64_t`): 消息的唯一ID。
- **返回值**: `std::optional<Message>` - 如果找到，返回包含`Message`对象的`optional`；否则返回`std::nullopt`。

