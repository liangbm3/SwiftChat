#include "room_repository.hpp"
#include "../utils/logger.hpp"
#include <chrono>
#include <random>
#include <sstream>

RoomRepository::RoomRepository(DatabaseConnection* db_conn) : db_conn_(db_conn) {}

std::optional<Room> RoomRepository::createRoom(const std::string &name, const std::string &description, const std::string &creator_id) {
    if (!db_conn_ || !db_conn_->isConnected()) {
        return std::nullopt;
    }

    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    
    // 1. 生成一个新的、唯一的房间ID
    std::string room_id = generateRoomId();

    LOG_INFO << "createRoom: room_id=" << room_id << ", name=" << name << ", description=" << description << ", creator_id=" << creator_id;

    const char *sql = "INSERT INTO rooms (id, name, description, creator_id, created_at) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        LOG_ERROR << "Failed to prepare statement for createRoom: " << sqlite3_errmsg(db_conn_->getDb());
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, room_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, description.c_str(), -1, SQLITE_STATIC); // 使用传入的描述
    sqlite3_bind_text(stmt, 4, creator_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 5, std::chrono::system_clock::now().time_since_epoch().count());

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (success) {
        // 2. 如果插入成功，立即用ID把这个新房间查出来并返回
        auto result = getRoomById(room_id);
        if (result.has_value()) {
            LOG_INFO << "createRoom success, returning: " << result.value().toJson().dump();
        } else {
            LOG_ERROR << "createRoom: getRoomById failed for room_id: " << room_id;
        }
        return result;
    } else {
        // 3. 如果插入失败（比如房间名重复），则返回空
        LOG_ERROR << "Failed to execute statement for createRoom, possibly due to duplicate name.";
        return std::nullopt;
    }
}

bool RoomRepository::deleteRoom(const std::string &room_id)
{
    if (!db_conn_->isConnected()) return false;
    
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    const char *sql = "DELETE FROM rooms WHERE id = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_conn_->getDb());
        return false;
    }

    sqlite3_bind_text(stmt, 1, room_id.c_str(), -1, SQLITE_STATIC);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool RoomRepository::roomExists(const std::string &room_id)
{
    if (!db_conn_->isConnected()) return false;
    
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    const char *sql = "SELECT COUNT(*) FROM rooms WHERE id = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_conn_->getDb());
        return false;
    }

    sqlite3_bind_text(stmt, 1, room_id.c_str(), -1, SQLITE_STATIC);

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        exists = (sqlite3_column_int(stmt, 0) > 0);
    }

    sqlite3_finalize(stmt);
    return exists;
}

bool RoomRepository::updateRoom(const std::string &room_id, const std::string &name, const std::string &description)
{
    if (!db_conn_->isConnected()) return false;
    
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    const char *sql = "UPDATE rooms SET name = ?, description = ? WHERE id = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_conn_->getDb());
        return false;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, description.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, room_id.c_str(), -1, SQLITE_STATIC);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

std::vector<std::string> RoomRepository::getRooms()
{
    std::vector<std::string> rooms;
    if (!db_conn_->isConnected()) return rooms;
    
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    const char *sql = "SELECT name FROM rooms;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_conn_->getDb());
        return rooms;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        rooms.push_back(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
    }

    sqlite3_finalize(stmt);
    return rooms;
}

std::optional<Room> RoomRepository::getRoomById(const std::string &room_id) const
{
    // 1. 检查数据库连接
    if (!db_conn_ || !db_conn_->isConnected())
    {
        return std::nullopt;
    }

    // 2. 获取锁以保证线程安全
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());

    // 3. 准备SQL查询语句
    const char *sql = "SELECT id, name, description, creator_id, created_at FROM rooms WHERE id = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement for getRoomById: " << sqlite3_errmsg(db_conn_->getDb());
        return std::nullopt; // 准备失败，返回空
    }

    // 4. 绑定参数
    sqlite3_bind_text(stmt, 1, room_id.c_str(), -1, SQLITE_STATIC);

    // 5. 执行查询并处理结果
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        // 找到了匹配的房间，开始映射数据到JSON对象
        const unsigned char* id_col = sqlite3_column_text(stmt, 0);
        const unsigned char* name_col = sqlite3_column_text(stmt, 1);
        const unsigned char* desc_col = sqlite3_column_text(stmt, 2);
        const unsigned char* creator_id_col = sqlite3_column_text(stmt, 3);
        int64_t created_at_col = sqlite3_column_int64(stmt, 4);

        // 安全地转换字符串，确保非 NULL
        std::string id_str = id_col ? std::string(reinterpret_cast<const char*>(id_col)) : "";
        std::string name_str = name_col ? std::string(reinterpret_cast<const char*>(name_col)) : "";
        std::string desc_str = desc_col ? std::string(reinterpret_cast<const char*>(desc_col)) : "";
        std::string creator_id_str = creator_id_col ? std::string(reinterpret_cast<const char*>(creator_id_col)) : "";

        LOG_INFO << "getRoomById: id=" << id_str 
                 << ", name=" << name_str
                 << ", desc=" << desc_str
                 << ", creator=" << creator_id_str;

        // 创建Room对象
        Room room(id_str, name_str, desc_str, creator_id_str, created_at_col);

        LOG_INFO << "getRoomById constructed Room: " << room.toJson().dump();

        // 6. 释放语句句柄并返回结果
        sqlite3_finalize(stmt);
        return room; // C++会自动将 room 包装在 std::optional 中
    }
    else
    {
        // 未找到匹配的行 (sqlite3_step 返回 SQLITE_DONE) 或发生错误
        // 6. 释放语句句柄并返回空
        sqlite3_finalize(stmt);
        return std::nullopt; // 明确返回“未找到”
    }
}

bool RoomRepository::isRoomCreator(const std::string &room_id, const std::string &user_id)
{
    if (!db_conn_->isConnected()) return false;
    
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    const char *sql = "SELECT COUNT(*) FROM rooms WHERE id = ? AND creator_id = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_conn_->getDb());
        return false;
    }

    sqlite3_bind_text(stmt, 1, room_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user_id.c_str(), -1, SQLITE_STATIC);

    bool is_creator = false;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        is_creator = (sqlite3_column_int(stmt, 0) > 0);
    }

    sqlite3_finalize(stmt);
    return is_creator;
}

std::vector<nlohmann::json> RoomRepository::getRoomMembers(const std::string &room_id) const
{
    std::vector<nlohmann::json> members;
    if (!db_conn_ || !db_conn_->isConnected())
    {
        return members;
    }

    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());

    // 使用 JOIN 查询，同时从 room_members 和 users 表中获取信息
    const char *sql = "SELECT u.id, u.username, rm.joined_at FROM room_members rm "
                      "JOIN users u ON rm.user_id = u.id WHERE rm.room_id = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement for getRoomMembers: " << sqlite3_errmsg(db_conn_->getDb());
        return members;
    }

    sqlite3_bind_text(stmt, 1, room_id.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *user_id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        const char *username = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        int64_t joined_at = sqlite3_column_int64(stmt, 2);

        nlohmann::json member = {
            {"id", user_id},
            {"username", username},
            {"joined_at", joined_at}
        };
        members.push_back(member);
    }

    sqlite3_finalize(stmt);
    return members;
}

bool RoomRepository::addRoomMember(const std::string &room_id, const std::string &user_id)
{
    if (!db_conn_->isConnected()) return false;
    
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    const char *sql = "INSERT OR IGNORE INTO room_members (room_id, user_id, joined_at) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_conn_->getDb());
        return false;
    }

    sqlite3_bind_text(stmt, 1, room_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, std::chrono::system_clock::now().time_since_epoch().count());

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

std::vector<Room> RoomRepository::getUserJoinedRooms(const std::string &user_id) const
{
    std::vector<Room> joined_rooms;
    if (!db_conn_->isConnected()) return joined_rooms;
    
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    const char *sql = "SELECT r.id, r.name, r.description, r.creator_id, r.created_at "
                      "FROM rooms r "
                      "JOIN room_members rm ON r.id = rm.room_id "
                      "WHERE rm.user_id = ?;";
    
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_conn_->getDb());
        return joined_rooms;
    }

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Room room;
        room.setId(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        room.setName(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        room.setDescription(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        room.setCreatorId(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        room.setCreatedAt(sqlite3_column_int64(stmt, 4));
        
        joined_rooms.push_back(room);
    }

    sqlite3_finalize(stmt);
    return joined_rooms;
}

bool RoomRepository::removeRoomMember(const std::string &room_id, const std::string &user_id)
{
    if (!db_conn_->isConnected()) return false;
    
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    const char *sql = "DELETE FROM room_members WHERE room_id = ? AND user_id = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db_conn_->getDb());
        return false;
    }

    sqlite3_bind_text(stmt, 1, room_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user_id.c_str(), -1, SQLITE_STATIC);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

std::string RoomRepository::generateRoomId()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    ss << "room_";
    for (int i = 0; i < 8; ++i)
    {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

std::optional<std::string> RoomRepository::getRoomIdByName(const std::string &room_name) const
{
    LOG_INFO << "getRoomIdByName called with room_name: '" << room_name << "'";
    
    // 1. 检查数据库连接
    if (!db_conn_ || !db_conn_->isConnected())
    {
        LOG_ERROR << "Database connection is null or not connected";
        return std::nullopt;
    }

    // 2. 获取锁以保证线程安全
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());

    // 3. 准备SQL查询语句
    const char *sql = "SELECT id FROM rooms WHERE name = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement for getRoomIdByName: " << sqlite3_errmsg(db_conn_->getDb());
        return std::nullopt;
    }

    // 4. 绑定参数
    sqlite3_bind_text(stmt, 1, room_name.c_str(), -1, SQLITE_STATIC);
    LOG_INFO << "Executing SQL query with room_name: '" << room_name << "'";

    // 5. 执行查询并处理结果
    int step_result = sqlite3_step(stmt);
    LOG_INFO << "SQLite step result: " << step_result << " (SQLITE_ROW=" << SQLITE_ROW << ", SQLITE_DONE=" << SQLITE_DONE << ")";
    
    if (step_result == SQLITE_ROW)
    {
        // 找到了匹配的房间，获取房间ID
        const unsigned char* id_col = sqlite3_column_text(stmt, 0);
        std::string room_id = reinterpret_cast<const char*>(id_col);

        LOG_INFO << "Found room ID: '" << room_id << "' for room name: '" << room_name << "'";
        
        // 6. 释放语句句柄并返回结果
        sqlite3_finalize(stmt);
        return room_id;
    }
    else
    {
        // 未找到匹配的房间名
        LOG_WARN << "No room found with name: '" << room_name << "'";
        sqlite3_finalize(stmt);
        return std::nullopt;
    }
}

std::vector<Room> RoomRepository::getAllRooms()
{
    std::vector<Room> rooms;
    if (!db_conn_->isConnected()) return rooms;
    
    std::lock_guard<std::recursive_mutex> lock(db_conn_->getMutex());
    const char *sql = "SELECT id, name, description, creator_id, created_at "
                      "FROM rooms ORDER BY created_at DESC;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_conn_->getDb(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR << "Failed to prepare statement for getAllRooms: " << sqlite3_errmsg(db_conn_->getDb());
        return rooms;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Room room(
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)),
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)),
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)),
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)),
            sqlite3_column_int64(stmt, 4)
        );
        rooms.push_back(room);
    }

    sqlite3_finalize(stmt);
    return rooms;
}
