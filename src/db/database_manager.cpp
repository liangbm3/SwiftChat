#include "database_manager.hpp"
#include "../utils/logger.hpp"
#include <sstream>
#include <chrono>


DatabaseManager::DatabaseManager(const std::string &db_path) : db_path_(db_path), db_(nullptr)
{
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);   // 获取锁
        if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK) // 打开数据库连接
        {
            LOG_ERROR << "Can't open database: " << sqlite3_errmsg(db_);
            return;
        }
        LOG_INFO << "Opened database successfully";
    }
    if (initializeTables())
    {
        LOG_INFO << "Initialized tables successfully";
    }
    else
    {
        LOG_ERROR << "Failed to initialize tables";
        sqlite3_close(db_); // 关闭数据库连接
        db_ = nullptr;      // 设置db_为nullptr，表示数据库未打开
        return;
    }
}


DatabaseManager::~DatabaseManager()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (db_)
    {
        LOG_INFO << "Closing database connection";
        sqlite3_close(db_);
    }
}

//初始化数据表
bool DatabaseManager::initializeTables()
{
    // 用户表
    // +--------------------+-----------+------------------------------------------------------------------+
    // | 字段名 (Column)    | 数据类型(Type) | 约束/描述 (Constraints/Description)                              |
    // +--------------------+-----------+------------------------------------------------------------------+
    // | username           | TEXT      | 主键 (PRIMARY KEY)。用户的唯一标识符，不能重复或为空。           |
    // | password_hash      | TEXT      | 不能为空 (NOT NULL)。存储用户密码经过哈希计算后的字符串。        |
    // | created_at         | INTEGER   | 不能为空 (NOT NULL)。存储账户创建时间的Unix时间戳。              |
    // | is_online          | INTEGER   | 默认值为 0 (DEFAULT 0)。标记用户是否在线 (0=离线, 1=在线)。     |
    // | last_active_time   | INTEGER   | 默认值为 0 (DEFAULT 0)。存储用户最后一次活跃时间的Unix时间戳。     |
    // +--------------------+-----------+------------------------------------------------------------------+
    const char *create_users_table =
        "CREATE TABLE IF NOT EXISTS users ("
        "username TEXT PRIMARY KEY,"
        "password_hash TEXT NOT NULL,"
        "created_at INTEGER NOT NULL,"
        "is_online INTEGER DEFAULT 0,"
        "last_active_time INTEGER DEFAULT 0);";

    // 聊天室表
    // +--------------+-----------+-------------------------------------------------------------+
    // | 字段名 (Column) | 数据类型(Type) | 约束/描述 (Constraints/Description)                         |
    // +--------------+-----------+-------------------------------------------------------------+
    // | name         | TEXT      | 主键 (PRIMARY KEY)。聊天室的唯一名称。                      |
    // | creator      | TEXT      | 不能为空 (NOT NULL)。指向 users.username 的外键，记录创建者。 |
    // | created_at   | INTEGER   | 不能为空 (NOT NULL)。聊天室创建时间的Unix时间戳。         |
    // +--------------+-----------+-------------------------------------------------------------+
    const char *create_rooms_table =
        "CREATE TABLE IF NOT EXISTS rooms ("
        "name TEXT PRIMARY KEY,"
        "creator TEXT NOT NULL,"
        "created_at INTEGER NOT NULL,"
        "FOREIGN KEY(creator) REFERENCES users(username));";

    // 聊天室成员表
    // +--------------+-----------+-------------------------------------------------------------+
    // | 字段名 (Column) | 数据类型(Type) | 约束/描述 (Constraints/Description)                         |
    // +--------------+-----------+-------------------------------------------------------------+
    // | room_name    | TEXT      | 复合主键之一，指向 rooms.name 的外键。                      |
    // | username     | TEXT      | 复合主键之一，指向 users.username 的外键。                  |
    // | joined_at    | INTEGER   | 不能为空 (NOT NULL)。用户加入房间时间的Unix时间戳。       |
    // +--------------+-----------+-------------------------------------------------------------+
    const char *create_room_members_table =
        "CREATE TABLE IF NOT EXISTS room_members ("
        "room_name TEXT NOT NULL,"
        "username TEXT NOT NULL,"
        "joined_at INTEGER NOT NULL,"
        "PRIMARY KEY(room_name, username),"
        "FOREIGN KEY(room_name) REFERENCES rooms(name),"
        "FOREIGN KEY(username) REFERENCES users(username));";

    // 消息表
    // +--------------+-----------+-------------------------------------------------------------+
    // | 字段名 (Column) | 数据类型(Type) | 约束/描述 (Constraints/Description)                         |
    // +--------------+-----------+-------------------------------------------------------------+
    // | id           | INTEGER   | 主键 (PRIMARY KEY)，自动递增，为每条消息提供唯一ID。      |
    // | room_name    | TEXT      | 不能为空 (NOT NULL)，指向 rooms.name 的外键，记录消息所在房间。 |
    // | username     | TEXT      | 不能为空 (NOT NULL)，指向 users.username 的外键，记录发送者。 |
    // | content      | TEXT      | 不能为空 (NOT NULL)。消息的文本内容。                      |
    // | timestamp    | INTEGER   | 不能为空 (NOT NULL)。消息发送时间的Unix时间戳。           |
    // +--------------+--
    const char *create_messages_table =
        "CREATE TABLE IF NOT EXISTS messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "room_name TEXT NOT NULL,"
        "username TEXT NOT NULL,"
        "content TEXT NOT NULL,"
        "timestamp INTEGER NOT NULL,"
        "FOREIGN KEY(room_name) REFERENCES rooms(name),"
        "FOREIGN KEY(username) REFERENCES users(username));";

    return executeQuery(create_users_table) &&
           executeQuery(create_rooms_table) &&
           executeQuery(create_room_members_table) &&
           executeQuery(create_messages_table);
}

//执行sql语句
bool DatabaseManager::executeQuery(const std::string &query)
{
    
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    char *err_msg = nullptr; // 指向错误信息的空指针
    int rc = sqlite3_exec(db_, query.c_str(), nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK)
    {
        LOG_ERROR << "SQL error: " << err_msg;
        sqlite3_free(err_msg); // 释放错误信息占用的内存
        return false;
    }
    return true;
}


bool DatabaseManager::createUser(const std::string &username, const std::string &password_hash)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const char* sql="INSERT INTO users (username, password_hash, created_at) VALUES(?, ?, ?);";
    //准备sql语句
    sqlite3_stmt *stmt;
    //参数：数据库连接对象、SQL查询字符串，字符串的长度（-1表示自动计算）
    //stmt用于存储编译后的SQL语句对象，nullptr表示不需要返回未处理的SQL部分
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_);
        return false;
    }
    //绑定参数
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password_hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, std::chrono::system_clock::now().time_since_epoch().count());
    //执行SQL语句，并检查是否成功插入
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);//释放资源
    return success;
}

bool DatabaseManager::validateUser(const std::string &username, const std::string &password_hash)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const char *sql = "SELECT COUNT(*) FROM users WHERE username = ? AND password_hash = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_);
        return false;
    }
    //绑定参数
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password_hash.c_str(), -1, SQLITE_STATIC);

    bool valid = false;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        valid = (sqlite3_column_int(stmt, 0) > 0);
    }

    sqlite3_finalize(stmt);
    return valid;
}

bool DatabaseManager::userExists(const std::string &username)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const char *sql = "SELECT COUNT(*) FROM users WHERE username = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR<< "Failed to prepare statement: " << sqlite3_errmsg(db_);
        return false;
    }
    //绑定参数
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        exists = (sqlite3_column_int(stmt, 0) > 0);
    }
    sqlite3_finalize(stmt);
    return exists;
}


bool DatabaseManager::createRoom(const std::string &name, const std::string &creator)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const char *sql = "INSERT INTO rooms (name, creator, created_at) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_);
        return false;
    }
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, creator.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, std::chrono::system_clock::now().time_since_epoch().count());
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool DatabaseManager::deleteRoom(const std::string &name)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const char *sql = "DELETE FROM rooms WHERE name = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_);
        return false;
    }
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

std::vector<std::string> DatabaseManager::getRooms()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<std::string> rooms;
    const char *query = "SELECT name FROM rooms;";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, query, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_);
        return rooms;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        rooms.push_back(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
    }

    sqlite3_finalize(stmt);
    return rooms;
}

bool DatabaseManager::addRoomMember(const std::string &room_name, const std::string &username)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    // 首先检查用户是否已经在房间中
    const char *check_sql = "SELECT COUNT(*) FROM room_members WHERE room_name = ? AND username = ?;";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, check_sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_);
        return false;
    }

    sqlite3_bind_text(stmt, 1, room_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_STATIC);

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        exists = (sqlite3_column_int(stmt, 0) > 0);
    }

    sqlite3_finalize(stmt);

    // 如果用户已经在房间中，直接返回true
    if (exists)
    {
        return true;
    }

    // 用户不在房间中，执行插入操作
    const char *insert_sql = "INSERT INTO room_members (room_name, username, joined_at) VALUES (?, ?, ?);";
    if (sqlite3_prepare_v2(db_, insert_sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_);
        return false;
    }
    sqlite3_bind_text(stmt, 1, room_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, std::chrono::system_clock::now().time_since_epoch().count());
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}


bool DatabaseManager::removeRoomMember(const std::string &room_name, const std::string &username)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const char *sql = "DELETE FROM room_members WHERE room_name = ? AND username = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_);
        return false;
    }
    sqlite3_bind_text(stmt, 1, room_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_STATIC);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}


std::vector<std::string> DatabaseManager::getRoomMembers(const std::string &room_name)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<std::string> members;
    const char *sql = "SELECT username FROM room_members WHERE room_name = ?;";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_);
        return members;
    }

    sqlite3_bind_text(stmt, 1, room_name.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        members.push_back(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
    }

    sqlite3_finalize(stmt);
    return members;
}

bool DatabaseManager::saveMessage(const std::string &room_name, const std::string &username,
                                  const std::string &content, int64_t timestamp)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const char *sql = "INSERT INTO messages (room_name, username, content, timestamp) VALUES (?, ?, ?, ?);";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_);
        return false;
    }
    sqlite3_bind_text(stmt, 1, room_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, timestamp);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}


std::vector<nlohmann::json> DatabaseManager::getMessages(const std::string &room_name, int64_t since)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<nlohmann::json> messages;
    const char *sql = "SELECT username, content, timestamp FROM messages WHERE room_name = ? AND timestamp >= ? ORDER BY timestamp ASC;";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_);
        return messages;
    }

    sqlite3_bind_text(stmt, 1, room_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, since);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        nlohmann::json msg;
        msg["username"] = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        msg["content"] = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        msg["timestamp"] = sqlite3_column_int64(stmt, 2);
        messages.push_back(msg);
    }

    sqlite3_finalize(stmt);
    return messages;
}


bool DatabaseManager::setUserOnlineStatus(const std::string &username, bool is_online)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const char *sql = "UPDATE users SET is_online = ?, last_active_time = ? WHERE username = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_);
        return false;
    }
    sqlite3_bind_int(stmt, 1, is_online ? 1 : 0);
    sqlite3_bind_int64(stmt, 2, std::chrono::system_clock::now().time_since_epoch().count());
    sqlite3_bind_text(stmt, 3, username.c_str(), -1, SQLITE_STATIC);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

//更新指定用户的最后活跃时间
bool DatabaseManager::updateUserLastActiveTime(const std::string &username)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const char *sql = "UPDATE users SET last_active_time = ? WHERE username = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_);
        return false;
    }
    sqlite3_bind_int64(stmt, 1, std::chrono::system_clock::now().time_since_epoch().count());
    sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_STATIC);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

//检查并更新长时间未活跃的用户，将其在线状态设置为离线。
bool DatabaseManager::checkAndUpdateInactiveUsers(int64_t timeout_ms)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    int64_t current_time = std::chrono::system_clock::now().time_since_epoch().count();
    int64_t timeout_time = current_time - timeout_ms;

    const char *sql = "UPDATE users SET is_online = 0 WHERE is_online = 1 AND last_active_time < ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_);
        return false;
    }
    sqlite3_bind_int64(stmt, 1, timeout_time);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}


std::vector<User> DatabaseManager::getAllUsers()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<User> users;
    const char *query = "SELECT username, password_hash, is_online FROM users;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_, query, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_);
        return users;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *username = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        const char *password = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        bool is_online = sqlite3_column_int(stmt, 2) > 0;
        users.push_back({std::string(username), std::string(password), is_online});
    }

    sqlite3_finalize(stmt);
    return users;
}
