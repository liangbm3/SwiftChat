#include "user_repository.hpp"
#include "../utils/logger.hpp"
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>

UserRepository::UserRepository(DatabaseConnection* db_conn) : db_conn_(db_conn) {}

bool UserRepository::createUser(const std::string &username, const std::string &password_hash)
{
    if (!db_conn_->isConnected()) return false;//如果数据库未连接，直接返回失败
    
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());//获取连接锁
    std::string user_id = generateUserId();//生成用户ID

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

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
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

bool UserRepository::userExists(const std::string &username)
{
    if (!db_conn_->isConnected()) return false;
    
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    const char *sql = "SELECT COUNT(*) FROM users WHERE username = ?;";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_conn_->getDb());
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        exists = (sqlite3_column_int(stmt, 0) > 0);
    }

    sqlite3_finalize(stmt);
    return exists;
}

bool UserRepository::setUserOnlineStatus(const std::string &username, bool is_online)
{
    if (!db_conn_->isConnected()) return false;
    
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    const char *sql = "UPDATE users SET is_online = ? WHERE username = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_conn_->getDb());
        return false;
    }

    sqlite3_bind_int(stmt, 1, is_online ? 1 : 0);
    sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_STATIC);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool UserRepository::updateUserLastActiveTime(const std::string &username)
{
    if (!db_conn_->isConnected()) return false;
    
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    int64_t current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    const char *sql = "UPDATE users SET last_active_time = ? WHERE username = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_conn_->getDb());
        return false;
    }

    sqlite3_bind_int64(stmt, 1, current_time);
    sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_STATIC);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool UserRepository::checkAndUpdateInactiveUsers(int64_t timeout_ms)
{
    if (!db_conn_->isConnected()) return false;
    
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    int64_t current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t cutoff_time = current_time - timeout_ms;

    const char *sql = "UPDATE users SET is_online = 0 WHERE is_online = 1 AND last_active_time < ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_conn_->getDb());
        return false;
    }

    sqlite3_bind_int64(stmt, 1, cutoff_time);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

std::vector<User> UserRepository::getAllUsers() const
{
    std::vector<User> users;
    if (!db_conn_->isConnected()) return users;
    
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    const char *sql = "SELECT id, username, password_hash, is_online, last_active_time FROM users;";
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
        bool is_online = sqlite3_column_int(stmt, 3) > 0;
        int64_t last_active_time = sqlite3_column_int64(stmt, 4);

        User user(std::string(id), std::string(username), std::string(password),
                  is_online, last_active_time);
        users.push_back(user);
    }

    sqlite3_finalize(stmt);
    return users;
}

std::optional<User> UserRepository::getUserById(const std::string &user_id) const
{
    if (!db_conn_->isConnected()) return std::nullopt;

    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    const char *sql = "SELECT id, username, password_hash, is_online, last_active_time FROM users WHERE id = ?;";
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
        const char *id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        const char *username = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        const char *password = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        bool is_online = sqlite3_column_int(stmt, 3) > 0;
        int64_t last_active_time = sqlite3_column_int64(stmt, 4);

        // 构造并返回User对象。C++会自动将其包装在std::optional中
        User found_user(std::string(id), std::string(username), std::string(password),
                        is_online, last_active_time);
        sqlite3_finalize(stmt);
        return found_user;
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
    const char *sql = "SELECT id, username, password_hash, is_online, last_active_time FROM users WHERE username = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_conn_->getDb());
        return User();
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        const char *username_col = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        const char *password = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        bool is_online = sqlite3_column_int(stmt, 3) > 0;
        int64_t last_active_time = sqlite3_column_int64(stmt, 4);

       User found_user(std::string(id), std::string(username_col), std::string(password),
                        is_online, last_active_time);
        sqlite3_finalize(stmt);
        return found_user;
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
