#include "database_connection.hpp"
#include <chrono>

DatabaseConnection::DatabaseConnection(const std::string &db_path) : db_path_(db_path), db_(nullptr)
{
    {
        //进入临界区，加锁
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK)//尝试打开数据库
        {
            LOG_ERROR << "Can't open database: " << sqlite3_errmsg(db_);
            return;
        }
        LOG_INFO << "Opened database successfully";
    }
    
    //如果连接成功则初始化表
    if (initializeTables())
    {
        LOG_INFO << "Initialized tables successfully";
    }
    else
    {
        LOG_ERROR << "Failed to initialize tables";
        sqlite3_close(db_);
        db_ = nullptr;
        return;
    }
}

DatabaseConnection::~DatabaseConnection()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (db_)
    {
        LOG_INFO << "Closing database connection";
        sqlite3_close(db_);
    }
}

bool DatabaseConnection::executeQuery(const std::string &query)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    char *err_msg = nullptr;
    int rc = sqlite3_exec(db_, query.c_str(), nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK)
    {
        LOG_ERROR << "SQL error: " << err_msg;
        sqlite3_free(err_msg);
        return false;
    }
    return true;
}

bool DatabaseConnection::initializeTables()
{
    return createUsersTable() &&
           createRoomsTable() &&
           createRoomMembersTable() &&
           createMessagesTable() &&
           createIndexes();
}

bool DatabaseConnection::createUsersTable()
{
    const char *create_users_table =
        "CREATE TABLE IF NOT EXISTS users ("
        "id TEXT PRIMARY KEY,"
        "username TEXT UNIQUE NOT NULL,"
        "password_hash TEXT NOT NULL,"
        "created_at INTEGER NOT NULL,"
        "is_online INTEGER DEFAULT 0,"
        "last_active_time INTEGER DEFAULT 0);";
    
    return executeQuery(create_users_table);
}

bool DatabaseConnection::createRoomsTable()
{
    const char *create_rooms_table =
        "CREATE TABLE IF NOT EXISTS rooms ("
        "id TEXT PRIMARY KEY,"
        "name TEXT UNIQUE NOT NULL,"
        "description TEXT DEFAULT '',"
        "creator_id TEXT NOT NULL,"
        "created_at INTEGER NOT NULL,"
        "FOREIGN KEY(creator_id) REFERENCES users(id));";
    
    return executeQuery(create_rooms_table);
}

bool DatabaseConnection::createRoomMembersTable()
{
    const char *create_room_members_table =
        "CREATE TABLE IF NOT EXISTS room_members ("
        "room_id TEXT NOT NULL,"
        "user_id TEXT NOT NULL,"
        "joined_at INTEGER NOT NULL,"
        "PRIMARY KEY(room_id, user_id),"
        "FOREIGN KEY(room_id) REFERENCES rooms(id),"
        "FOREIGN KEY(user_id) REFERENCES users(id));";
    
    return executeQuery(create_room_members_table);
}

bool DatabaseConnection::createMessagesTable()
{
    const char *create_messages_table =
        "CREATE TABLE IF NOT EXISTS messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "room_id TEXT NOT NULL,"
        "user_id TEXT NOT NULL,"
        "content TEXT NOT NULL,"
        "timestamp INTEGER NOT NULL,"
        "FOREIGN KEY(room_id) REFERENCES rooms(id),"
        "FOREIGN KEY(user_id) REFERENCES users(id));";
    
    return executeQuery(create_messages_table);
}

bool DatabaseConnection::createIndexes()
{
    const char *create_username_index = "CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);";
    const char *create_room_name_index = "CREATE INDEX IF NOT EXISTS idx_rooms_name ON rooms(name);";
    
    return executeQuery(create_username_index) && executeQuery(create_room_name_index);
}
