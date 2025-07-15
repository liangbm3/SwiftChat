#include "message_repository.hpp"
#include "../utils/logger.hpp"
#include <chrono>

MessageRepository::MessageRepository(DatabaseConnection* db_conn) : db_conn_(db_conn) {}

bool MessageRepository::saveMessage(const std::string &room_name, const std::string &username,
                                   const std::string &content, int64_t timestamp)
{
    if (!db_conn_->isConnected()) return false;
    
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    
    // 获取房间ID和用户ID
    const char *get_ids_sql = 
        "SELECT r.id, u.id FROM rooms r, users u WHERE r.name = ? AND u.username = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_conn_->getDb(), get_ids_sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_conn_->getDb());
        return false;
    }

    sqlite3_bind_text(stmt, 1, room_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_STATIC);

    std::string room_id, user_id;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        room_id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        user_id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    }
    sqlite3_finalize(stmt);

    if (room_id.empty() || user_id.empty())
    {
        LOG_ERROR << "Room or user not found";
        return false;
    }

    return saveMessageById(room_id, user_id, content, timestamp);
}

std::vector<nlohmann::json> MessageRepository::getMessages(const std::string &room_name, int64_t since)
{
    std::vector<nlohmann::json> messages;
    if (!db_conn_->isConnected()) return messages;
    
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    
    // 获取房间ID
    const char *get_room_sql = "SELECT id FROM rooms WHERE name = ?;";
    sqlite3_stmt *stmt;
    std::string room_id;

    if (sqlite3_prepare_v2(db_conn_->getDb(), get_room_sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_conn_->getDb());
        return messages;
    }

    sqlite3_bind_text(stmt, 1, room_name.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        room_id = std::string(id);
    }
    sqlite3_finalize(stmt);

    if (room_id.empty())
    {
        LOG_ERROR << "Room not found: " << room_name;
        return messages;
    }

    return getMessagesById(room_id, 50, since);
}

bool MessageRepository::saveMessageById(const std::string &room_id, const std::string &user_id,
                                       const std::string &content, int64_t timestamp)
{
    if (!db_conn_->isConnected()) return false;
    
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    const char *sql = "INSERT INTO messages (room_id, user_id, content, timestamp) VALUES (?, ?, ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_conn_->getDb());
        return false;
    }

    sqlite3_bind_text(stmt, 1, room_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, timestamp);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

std::vector<nlohmann::json> MessageRepository::getMessagesById(const std::string &room_id, int limit,
                                                              int64_t before_timestamp)
{
    std::vector<nlohmann::json> messages;
    if (!db_conn_->isConnected()) return messages;
    
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    
    std::string sql = 
        "SELECT m.id, m.content, m.timestamp, u.id, u.username "
        "FROM messages m "
        "JOIN users u ON m.user_id = u.id "
        "WHERE m.room_id = ?";
    
    if (before_timestamp > 0)
    {
        sql += " AND m.timestamp >= ?";
    }
    
    sql += " ORDER BY m.timestamp ASC";
    
    if (limit > 0)
    {
        sql += " LIMIT ?";
    }

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_conn_->getDb(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_conn_->getDb());
        return messages;
    }

    int param_index = 1;
    sqlite3_bind_text(stmt, param_index++, room_id.c_str(), -1, SQLITE_STATIC);
    
    if (before_timestamp > 0)
    {
        sqlite3_bind_int64(stmt, param_index++, before_timestamp);
    }
    
    if (limit > 0)
    {
        sqlite3_bind_int(stmt, param_index++, limit);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        nlohmann::json message;
        message["id"] = sqlite3_column_int64(stmt, 0);
        message["content"] = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        message["timestamp"] = sqlite3_column_int64(stmt, 2);
        message["user_id"] = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        message["username"] = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
        messages.push_back(message);
    }

    sqlite3_finalize(stmt);
    return messages;
}
