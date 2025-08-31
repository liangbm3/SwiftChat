#include "room_repository.hpp"

#include "../mysql_statement.hpp"
#include "../../utils/logger.hpp"

namespace db {

RoomRepository::RoomRepository(ConnectionPool &pool) : pool_(pool) {}

std::optional<int64_t> RoomRepository::createRoom(const std::string &name, int64_t creator_id) {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return std::nullopt;
  }

  const char *sql = "INSERT INTO rooms (name, description, creator_id) VALUES (?, '', ?)";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindString(0, name) || !stmt.bindLong(1, creator_id)) {
    LOG_ERROR << "Failed to bind parameters for createRoom";
    return std::nullopt;
  }

  if (stmt.executeUpdate()) {
    int64_t room_id = static_cast<int64_t>(stmt.getLastInsertId());
    LOG_INFO << "Room '" << name << "' created with ID: " << room_id;
    return room_id;
  }

  LOG_ERROR << "Failed to create room '" << name << "'";
  return std::nullopt;
}

bool RoomRepository::deleteRoom(int64_t room_id) {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return false;
  }

  const char *sql = "DELETE FROM rooms WHERE id = ?";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, room_id)) {
    LOG_ERROR << "Failed to bind parameters for deleteRoom";
    return false;
  }

  if (stmt.executeUpdate() && stmt.getAffectedRows() >= 1) {
    LOG_INFO << "Room with ID " << room_id << " deleted successfully";
    return true;
  }

  LOG_ERROR << "Failed to delete room with ID: " << room_id;
  return false;
}

bool RoomRepository::roomExists(int64_t room_id) const {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return false;
  }

  const char *sql = "SELECT id FROM rooms WHERE id = ?";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, room_id)) {
    LOG_ERROR << "Failed to bind room ID parameter";
    return false;
  }

  if (stmt.executeQuery()) {
    return (stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS);
  }

  LOG_ERROR << "Failed to check if room exists with ID: " << room_id;
  return false;
}

bool RoomRepository::updateRoom(int64_t room_id, const std::string &name,
                                const std::string &description) {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return false;
  }

  // 先检查房间是否存在
  const char *check_sql = "SELECT id FROM rooms WHERE id = ?";
  MySQLStatement check_stmt(connection->getRawConnection(), check_sql);
  
  if (!check_stmt.bindLong(0, room_id)) {
    LOG_ERROR << "Failed to bind room ID parameter for existence check";
    return false;
  }
  
  bool room_exists = false;
  if (check_stmt.executeQuery()) {
    room_exists = (check_stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS);
  } else {
    LOG_ERROR << "Failed to execute existence check query for room ID: " << room_id;
    return false;
  }
  
  if (!room_exists) {
    LOG_ERROR << "Room with ID " << room_id << " does not exist";
    return false;
  }

  // 执行更新
  const char *sql = "UPDATE rooms SET name = ?, description = ? WHERE id = ?";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindString(0, name) || 
      !stmt.bindString(1, description) ||
      !stmt.bindLong(2, room_id)) {
    LOG_ERROR << "Failed to bind parameters for updateRoom";
    return false;
  }

  if (stmt.executeUpdate()) {
    LOG_INFO << "Room with ID " << room_id << " updated successfully";
    return true;
  }

  LOG_ERROR << "Failed to update room with ID: " << room_id;
  return false;
}

std::vector<std::string> RoomRepository::getAllRoomNames() const {
  std::vector<std::string> room_names;
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return room_names;
  }

  const char *sql = "SELECT name FROM rooms ORDER BY created_at DESC";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (stmt.executeQuery()) {
    while (stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS) {
      std::string name = stmt.getString(0);
      room_names.push_back(name);
    }
  } else {
    LOG_ERROR << "Failed to execute query: " << sql;
  }

  return room_names;
}

std::vector<Room> RoomRepository::getAllRooms() const {
  std::vector<Room> rooms;
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return rooms;
  }

  const char *sql = 
    "SELECT id, name, description, creator_id, created_at "
    "FROM rooms "
    "ORDER BY created_at DESC";
  
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (stmt.executeQuery()) {
    while (stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS) {
      Room room(
        stmt.getLong(0),      // id
        stmt.getString(1),    // name
        stmt.getString(2),    // description
        stmt.getLong(3),      // creator_id
        stmt.getString(4)     // created_at
      );
      
      rooms.push_back(room);
    }
  } else {
    LOG_ERROR << "Failed to execute query: " << sql;
  }

  return rooms;
}

std::optional<Room> RoomRepository::getRoom(int64_t room_id) const {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return std::nullopt;
  }

  const char *sql = "SELECT id, name, description, creator_id, created_at FROM rooms WHERE id = ?";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, room_id)) {
    LOG_ERROR << "Failed to bind room ID parameter for getRoom";
    return std::nullopt;
  }

  if (stmt.executeQuery() && stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS) {
    Room room(
      stmt.getLong(0),      // id
      stmt.getString(1),    // name
      stmt.getString(2),    // description
      stmt.getLong(3),      // creator_id
      stmt.getString(4)     // created_at
    );
    
    return room;
  }

  LOG_ERROR << "Failed to find room with ID: " << room_id;
  return std::nullopt;
}

std::optional<int64_t> RoomRepository::getRoomIdByName(const std::string &room_name) const {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return std::nullopt;
  }

  const char *sql = "SELECT id FROM rooms WHERE name = ?";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindString(0, room_name)) {
    LOG_ERROR << "Failed to bind room name parameter";
    return std::nullopt;
  }

  if (stmt.executeQuery() && stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS) {
    return stmt.getLong(0);
  }

  LOG_ERROR << "Failed to find room with name: " << room_name;
  return std::nullopt;
}

std::optional<Room> RoomRepository::getRoom(const std::string &room_name) const {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection for getRoom(name)";
    return std::nullopt;
  }

  const char *sql = "SELECT id, name, description, creator_id, created_at FROM rooms WHERE name = ?";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindString(0, room_name)) {
    LOG_ERROR << "Failed to bind room name parameter for getRoom";
    return std::nullopt;
  }

  if (stmt.executeQuery() && stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS) {
    Room room(
      stmt.getLong(0),      // id
      stmt.getString(1),    // name
      stmt.getString(2),    // description
      stmt.getLong(3),      // creator_id
      stmt.getString(4)     // created_at
    );
    
    return room;
  }

  LOG_ERROR << "Failed to find room with name: " << room_name;
  return std::nullopt;
}

bool RoomRepository::isRoomCreator(int64_t user_id, int64_t room_id) const {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return false;
  }

  const char *sql = "SELECT id FROM rooms WHERE id = ? AND creator_id = ?";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, room_id) || !stmt.bindLong(1, user_id)) {
    LOG_ERROR << "Failed to bind parameters for isRoomCreator";
    return false;
  }

  if (stmt.executeQuery()) {
    return (stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS);
  }

  LOG_ERROR << "Failed to check if user " << user_id << " is creator of room " << room_id;
  return false;
}

std::vector<User> RoomRepository::getRoomMembers(int64_t room_id) const {
  std::vector<User> members;
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return members;
  }

  const char *sql = 
    "SELECT u.id, u.username, u.password_hash, u.status, u.last_seen "
    "FROM room_members rm "
    "JOIN users u ON rm.user_id = u.id "
    "WHERE rm.room_id = ? "
    "ORDER BY rm.joined_at ASC";
  
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, room_id)) {
    LOG_ERROR << "Failed to bind room ID parameter for getRoomMembers";
    return members;
  }

  if (stmt.executeQuery()) {
    while (stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS) {
      User user(
        stmt.getLong(0),      // id
        stmt.getString(1),    // username
        stmt.getString(2),    // password_hash
        stmt.getInt(3),       // status
        stmt.getString(4)     // last_seen
      );
      
      members.push_back(user);
    }
  } else {
    LOG_ERROR << "Failed to execute query for getRoomMembers";
  }

  return members;
}

bool RoomRepository::addRoomMember(int64_t room_id, int64_t user_id) {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return false;
  }

  // 检查用户是否已经是房间成员
  const char *check_sql = "SELECT user_id FROM room_members WHERE room_id = ? AND user_id = ?";
  MySQLStatement check_stmt(connection->getRawConnection(), check_sql);
  
  if (!check_stmt.bindLong(0, room_id) || !check_stmt.bindLong(1, user_id)) {
    LOG_ERROR << "Failed to bind parameters for membership check";
    return false;
  }
  
  if (check_stmt.executeQuery()) {
    if (check_stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS) {
      LOG_INFO << "User " << user_id << " is already a member of room " << room_id;
      return true; // 用户已经是成员，返回成功
    }
  } else {
    LOG_ERROR << "Failed to execute membership check query";
    return false;
  }

  // 添加用户到房间
  const char *sql = "INSERT INTO room_members (room_id, user_id) VALUES (?, ?)";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, room_id) || !stmt.bindLong(1, user_id)) {
    LOG_ERROR << "Failed to bind parameters for addRoomMember";
    return false;
  }

  if (stmt.executeUpdate()) {
    LOG_INFO << "User " << user_id << " added to room " << room_id;
    return true;
  }

  LOG_ERROR << "Failed to add user " << user_id << " to room " << room_id;
  return false;
}

bool RoomRepository::removeRoomMember(int64_t room_id, int64_t user_id) {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return false;
  }

  const char *sql = "DELETE FROM room_members WHERE room_id = ? AND user_id = ?";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, room_id) || !stmt.bindLong(1, user_id)) {
    LOG_ERROR << "Failed to bind parameters for removeRoomMember";
    return false;
  }

  if (stmt.executeUpdate() && stmt.getAffectedRows() >= 1) {
    LOG_INFO << "User " << user_id << " removed from room " << room_id;
    return true;
  }

  LOG_ERROR << "Failed to remove user " << user_id << " from room " << room_id << " (user may not be a member)";
  return false;
}

}  // namespace db
