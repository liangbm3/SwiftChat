#include "user_repository.hpp"
#include "../utils/logger.hpp"
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>

UserRepository::UserRepository(DatabaseConnection* db_conn) : db_conn_(db_conn) {}

bool UserRepository::createUser(const std::string &username, const std::string &password_hash)
{
    LOG_INFO << "Attempting to create user: " << username;
    
    if (!db_conn_->isConnected()) 
    {
        LOG_ERROR << "Database not connected when creating user: " << username;
        return false;//如果数据库未连接，直接返回失败
    }
    
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());//获取连接锁
    std::string user_id = generateUserId();//生成用户ID
    LOG_INFO << "Generated user ID: " << user_id << " for username: " << username;

    const char *sql = "INSERT INTO users (id, username, password_hash, created_at) VALUES(?, ?, ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_conn_->getDb());
        return false;
    }

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, password_hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, std::chrono::system_clock::now().time_since_epoch().count());

    LOG_INFO << "Executing INSERT statement for user: " << username;
    int step_result = sqlite3_step(stmt);
    bool success = (step_result == SQLITE_DONE);
    
    if (!success)
    {
        LOG_ERROR << "Failed to execute INSERT for user: " << username 
                  << ", SQLite error: " << sqlite3_errmsg(db_conn_->getDb())
                  << ", Step result: " << step_result;
    }
    else
    {
        LOG_INFO << "Successfully created user: " << username;
    }
    
    sqlite3_finalize(stmt);
    return success;
}

bool UserRepository::validateUser(const std::string &username, const std::string &password_hash)
{
    if (!db_conn_->isConnected()) return false;
    
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    const char *sql = "SELECT COUNT(*) FROM users WHERE username = ? AND password_hash = ?;";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_conn_->getDb());
        return false;
    }

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

bool UserRepository::userExists(const std::string &user_id)
{
    LOG_INFO << "userExists: Checking existence for user_id: " << user_id;
    
    if (!db_conn_->isConnected()) {
        LOG_ERROR << "userExists: Database not connected";
        return false;
    }
    
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    const char *sql = "SELECT COUNT(*) FROM users WHERE id = ?;";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "userExists: Failed to prepare statement: " << sqlite3_errmsg(db_conn_->getDb());
        return false;
    }

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_STATIC);

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int count = sqlite3_column_int(stmt, 0);
        exists = (count > 0);
        LOG_INFO << "userExists: Found " << count << " users with id: " << user_id;
    }
    else
    {
        LOG_ERROR << "userExists: Failed to execute query for user_id: " << user_id;
    }

    sqlite3_finalize(stmt);
    LOG_INFO << "userExists: Result for user_id " << user_id << " is " << (exists ? "true" : "false");
    return exists;
}


std::vector<User> UserRepository::getAllUsers() const
{
    std::vector<User> users;
    if (!db_conn_->isConnected()) return users;
    
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    const char *sql = "SELECT id, username, password_hash FROM users;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_conn_->getDb());
        return users;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        const char *username = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        const char *password = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        users.emplace_back(std::string(id), std::string(username), std::string(password));
    }

    sqlite3_finalize(stmt);
    return users;
}

std::optional<User> UserRepository::getUserById(const std::string &user_id) const
{
    if (!db_conn_->isConnected()) return std::nullopt;

    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    const char *sql = "SELECT id, username, password_hash FROM users WHERE id = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_conn_->getDb());
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        //确定找到了一行数据时，才构造 User 对象
        const unsigned char *id_col = sqlite3_column_text(stmt, 0);
        const unsigned char *username_col = sqlite3_column_text(stmt, 1);
        const unsigned char *password_col = sqlite3_column_text(stmt, 2);

        std::string id_str = id_col ? std::string(reinterpret_cast<const char*>(id_col)) : "";
        std::string username_str = username_col ? std::string(reinterpret_cast<const char*>(username_col)) : "";
        std::string password_str = password_col ? std::string(reinterpret_cast<const char*>(password_col)) : "";

        // 构造并返回User对象。C++会自动将其包装在std::optional中
        sqlite3_finalize(stmt);
        return User(id_str, username_str, password_str);
    }
    else
    {
        sqlite3_finalize(stmt); // 确保释放stmt资源
        LOG_ERROR << "User not found with ID: " << user_id; // 如果没有找到用户，记录错误日志
        return std::nullopt; // 如果没有找到用户，返回std::nullopt
    }
}

std::optional<User> UserRepository::getUserByUsername(const std::string &username) const
{
    if (!db_conn_->isConnected()) return std::nullopt;

    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    const char *sql = "SELECT id, username, password_hash FROM users WHERE username = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_conn_->getDb());
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const unsigned char *id_col = sqlite3_column_text(stmt, 0);
        const unsigned char *username_col = sqlite3_column_text(stmt, 1);
        const unsigned char *password_col = sqlite3_column_text(stmt, 2);

        std::string id_str = id_col ? std::string(reinterpret_cast<const char*>(id_col)) : "";
        std::string username_str = username_col ? std::string(reinterpret_cast<const char*>(username_col)) : "";
        std::string password_str = password_col ? std::string(reinterpret_cast<const char*>(password_col)) : "";

        sqlite3_finalize(stmt);
        return User(id_str, username_str, password_str);
    }
    else
    {
        sqlite3_finalize(stmt); // 确保释放stmt资源
        LOG_ERROR << "User not found with username: " << username; // 如果没有找到用户，记录错误日志
        return std::nullopt; // 如果没有找到用户，返回std::nullopt
    }
}

std::string UserRepository::generateUserId()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    ss << "user_";
    for (int i = 0; i < 8; ++i)
    {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}
