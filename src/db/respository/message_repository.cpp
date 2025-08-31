#include "message_repository.hpp"

#include "../mysql_statement.hpp"
#include "../../utils/logger.hpp"

namespace db {

MessageRepository::MessageRepository(ConnectionPool &pool) : pool_(pool) {}

// ================== 房间消息操作 ==================

std::optional<int64_t> MessageRepository::saveMessage(int64_t room_id, int64_t sender_id, 
                                                      const std::string &content) {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return std::nullopt;
  }

  const char *sql = "INSERT INTO messages (room_id, sender_id, content, created_at) VALUES (?, ?, ?, NOW())";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, room_id) || !stmt.bindLong(1, sender_id) || !stmt.bindString(2, content)) {
    LOG_ERROR << "Failed to bind parameters for saveMessage";
    return std::nullopt;
  }

  if (stmt.executeUpdate()) {
    int64_t message_id = static_cast<int64_t>(stmt.getLastInsertId());
    LOG_INFO << "Message saved with ID: " << message_id << " in room: " << room_id;
    return message_id;
  }

  LOG_ERROR << "Failed to save message in room: " << room_id;
  return std::nullopt;
}

bool MessageRepository::deleteMessage(int64_t message_id) {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return false;
  }

  const char *sql = "DELETE FROM messages WHERE id = ?";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, message_id)) {
    LOG_ERROR << "Failed to bind parameters for deleteMessage";
    return false;
  }

  if (stmt.executeUpdate() && stmt.getAffectedRows() >= 1) {
    LOG_INFO << "Message with ID " << message_id << " deleted successfully";
    return true;
  }

  LOG_ERROR << "Failed to delete message with ID: " << message_id;
  return false;
}

bool MessageRepository::messageExists(int64_t message_id) const {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return false;
  }

  const char *sql = "SELECT id FROM messages WHERE id = ?";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, message_id)) {
    LOG_ERROR << "Failed to bind message ID parameter";
    return false;
  }

  if (stmt.executeQuery()) {
    return (stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS);
  }

  LOG_ERROR << "Failed to check if message exists with ID: " << message_id;
  return false;
}

// ================== 房间消息查询 ==================

std::vector<Message> MessageRepository::getRoomMessages(int64_t room_id, int limit, int offset) const {
  std::vector<Message> messages;
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return messages;
  }

  const char *sql = 
    "SELECT m.id, m.room_id, m.sender_id, m.content, m.created_at, u.username "
    "FROM messages m "
    "JOIN users u ON m.sender_id = u.id "
    "WHERE m.room_id = ? "
    "ORDER BY m.created_at DESC "
    "LIMIT ? OFFSET ?";
  
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, room_id) || !stmt.bindInt(1, limit) || !stmt.bindInt(2, offset)) {
    LOG_ERROR << "Failed to bind parameters for getRoomMessages";
    return messages;
  }

  if (stmt.executeQuery()) {
    while (stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS) {
      Message message(
        stmt.getLong(0),      // id
        stmt.getLong(1),      // room_id
        stmt.getLong(2),      // sender_id
        stmt.getString(3),    // content
        stmt.getString(4),    // created_at
        stmt.getString(5)     // user_name
      );
      
      messages.push_back(message);
    }
    LOG_INFO << "Retrieved " << messages.size() << " messages for room " << room_id;
  } else {
    LOG_ERROR << "Failed to execute query for getRoomMessages";
  }

  return messages;
}

std::vector<Message> MessageRepository::getRoomMessagesAfter(int64_t room_id, 
                                                            const std::string &timestamp) const {
  std::vector<Message> messages;
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return messages;
  }

  const char *sql = 
    "SELECT m.id, m.room_id, m.sender_id, m.content, m.created_at, u.username "
    "FROM messages m "
    "JOIN users u ON m.sender_id = u.id "
    "WHERE m.room_id = ? AND m.created_at > ? "
    "ORDER BY m.created_at ASC";
  
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, room_id) || !stmt.bindString(1, timestamp)) {
    LOG_ERROR << "Failed to bind parameters for getRoomMessagesAfter";
    return messages;
  }

  if (stmt.executeQuery()) {
    while (stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS) {
      Message message(
        stmt.getLong(0),      // id
        stmt.getLong(1),      // room_id
        stmt.getLong(2),      // sender_id
        stmt.getString(3),    // content
        stmt.getString(4),    // created_at
        stmt.getString(5)     // user_name
      );
      
      messages.push_back(message);
    }
  } else {
    LOG_ERROR << "Failed to execute query for getRoomMessagesAfter";
  }

  return messages;
}

std::optional<Message> MessageRepository::getMessage(int64_t message_id) const {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection for getMessage";
    return std::nullopt;
  }

  const char *sql = 
    "SELECT m.id, m.room_id, m.sender_id, m.content, m.created_at, u.username "
    "FROM messages m "
    "JOIN users u ON m.sender_id = u.id "
    "WHERE m.id = ?";
  
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, message_id)) {
    LOG_ERROR << "Failed to bind message ID parameter for getMessage";
    return std::nullopt;
  }

  if (stmt.executeQuery() && stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS) {
    Message message(
      stmt.getLong(0),      // id
      stmt.getLong(1),      // room_id
      stmt.getLong(2),      // sender_id
      stmt.getString(3),    // content
      stmt.getString(4),    // created_at
      stmt.getString(5)     // user_name
    );
    
    return message;
  }

  LOG_ERROR << "Failed to find message with ID: " << message_id;
  return std::nullopt;
}

int64_t MessageRepository::getRoomMessageCount(int64_t room_id) const {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return 0;
  }

  const char *sql = "SELECT COUNT(*) FROM messages WHERE room_id = ?";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, room_id)) {
    LOG_ERROR << "Failed to bind room ID parameter for getRoomMessageCount";
    return 0;
  }

  if (stmt.executeQuery() && stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS) {
    return stmt.getLong(0);
  }

  LOG_ERROR << "Failed to get message count for room: " << room_id;
  return 0;
}

// ================== 私聊消息操作 ==================

std::optional<int64_t> MessageRepository::saveDirectMessage(int64_t sender_id, int64_t receiver_id,
                                                            const std::string &content) {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return std::nullopt;
  }

  const char *sql = "INSERT INTO direct_messages (sender_id, receiver_id, content, created_at) VALUES (?, ?, ?, NOW())";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, sender_id) || !stmt.bindLong(1, receiver_id) || !stmt.bindString(2, content)) {
    LOG_ERROR << "Failed to bind parameters for saveDirectMessage";
    return std::nullopt;
  }

  if (stmt.executeUpdate()) {
    int64_t message_id = static_cast<int64_t>(stmt.getLastInsertId());
    LOG_INFO << "Direct message saved with ID: " << message_id << " from " << sender_id << " to " << receiver_id;
    return message_id;
  }

  LOG_ERROR << "Failed to save direct message from " << sender_id << " to " << receiver_id;
  return std::nullopt;
}

bool MessageRepository::deleteDirectMessage(int64_t message_id) {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return false;
  }

  const char *sql = "DELETE FROM direct_messages WHERE id = ?";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, message_id)) {
    LOG_ERROR << "Failed to bind parameters for deleteDirectMessage";
    return false;
  }

  if (stmt.executeUpdate() && stmt.getAffectedRows() >= 1) {
    LOG_INFO << "Direct message with ID " << message_id << " deleted successfully";
    return true;
  }

  LOG_ERROR << "Failed to delete direct message with ID: " << message_id;
  return false;
}

bool MessageRepository::directMessageExists(int64_t message_id) const {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return false;
  }

  const char *sql = "SELECT id FROM direct_messages WHERE id = ?";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, message_id)) {
    LOG_ERROR << "Failed to bind message ID parameter";
    return false;
  }

  if (stmt.executeQuery()) {
    return (stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS);
  }

  LOG_ERROR << "Failed to check if direct message exists with ID: " << message_id;
  return false;
}

// ================== 私聊消息查询 ==================

std::vector<DirectMessage> MessageRepository::getDirectMessages(int64_t user1_id, int64_t user2_id,
                                                                int limit, int offset) const {
  std::vector<DirectMessage> messages;
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return messages;
  }

  const char *sql = 
    "SELECT id, sender_id, receiver_id, content, created_at "
    "FROM direct_messages "
    "WHERE (sender_id = ? AND receiver_id = ?) OR (sender_id = ? AND receiver_id = ?) "
    "ORDER BY created_at DESC "
    "LIMIT ? OFFSET ?";
  
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, user1_id) || !stmt.bindLong(1, user2_id) || 
      !stmt.bindLong(2, user2_id) || !stmt.bindLong(3, user1_id) ||
      !stmt.bindInt(4, limit) || !stmt.bindInt(5, offset)) {
    LOG_ERROR << "Failed to bind parameters for getDirectMessages";
    return messages;
  }

  if (stmt.executeQuery()) {
    while (stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS) {
      DirectMessage message(
        stmt.getLong(0),      // id
        stmt.getLong(1),      // sender_id
        stmt.getLong(2),      // receiver_id
        stmt.getString(3),    // content
        stmt.getString(4)     // created_at
      );
      
      messages.push_back(message);
    }
    LOG_INFO << "Retrieved " << messages.size() << " direct messages between users " << user1_id << " and " << user2_id;
  } else {
    LOG_ERROR << "Failed to execute query for getDirectMessages";
  }

  return messages;
}

std::vector<DirectMessage> MessageRepository::getDirectMessagesAfter(int64_t user1_id, int64_t user2_id,
                                                                     const std::string &timestamp) const {
  std::vector<DirectMessage> messages;
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return messages;
  }

  const char *sql = 
    "SELECT id, sender_id, receiver_id, content, created_at "
    "FROM direct_messages "
    "WHERE ((sender_id = ? AND receiver_id = ?) OR (sender_id = ? AND receiver_id = ?)) "
    "AND created_at > ? "
    "ORDER BY created_at ASC";
  
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, user1_id) || !stmt.bindLong(1, user2_id) || 
      !stmt.bindLong(2, user2_id) || !stmt.bindLong(3, user1_id) ||
      !stmt.bindString(4, timestamp)) {
    LOG_ERROR << "Failed to bind parameters for getDirectMessagesAfter";
    return messages;
  }

  if (stmt.executeQuery()) {
    while (stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS) {
      DirectMessage message(
        stmt.getLong(0),      // id
        stmt.getLong(1),      // sender_id
        stmt.getLong(2),      // receiver_id
        stmt.getString(3),    // content
        stmt.getString(4)     // created_at
      );
      
      messages.push_back(message);
    }
  } else {
    LOG_ERROR << "Failed to execute query for getDirectMessagesAfter";
  }

  return messages;
}

std::optional<DirectMessage> MessageRepository::getDirectMessage(int64_t message_id) const {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection for getDirectMessage";
    return std::nullopt;
  }

  const char *sql = "SELECT id, sender_id, receiver_id, content, created_at FROM direct_messages WHERE id = ?";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, message_id)) {
    LOG_ERROR << "Failed to bind message ID parameter for getDirectMessage";
    return std::nullopt;
  }

  if (stmt.executeQuery() && stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS) {
    DirectMessage message(
      stmt.getLong(0),      // id
      stmt.getLong(1),      // sender_id
      stmt.getLong(2),      // receiver_id
      stmt.getString(3),    // content
      stmt.getString(4)     // created_at
    );
    
    return message;
  }

  LOG_ERROR << "Failed to find direct message with ID: " << message_id;
  return std::nullopt;
}

int64_t MessageRepository::getDirectMessageCount(int64_t user1_id, int64_t user2_id) const {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return 0;
  }

  const char *sql = 
    "SELECT COUNT(*) FROM direct_messages "
    "WHERE (sender_id = ? AND receiver_id = ?) OR (sender_id = ? AND receiver_id = ?)";
  
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, user1_id) || !stmt.bindLong(1, user2_id) || 
      !stmt.bindLong(2, user2_id) || !stmt.bindLong(3, user1_id)) {
    LOG_ERROR << "Failed to bind parameters for getDirectMessageCount";
    return 0;
  }

  if (stmt.executeQuery() && stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS) {
    return stmt.getLong(0);
  }

  LOG_ERROR << "Failed to get direct message count between users " << user1_id << " and " << user2_id;
  return 0;
}

std::vector<int64_t> MessageRepository::getConversationPartners(int64_t user_id) const {
  std::vector<int64_t> partners;
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return partners;
  }

  const char *sql = 
    "SELECT DISTINCT "
    "CASE "
    "  WHEN sender_id = ? THEN receiver_id "
    "  ELSE sender_id "
    "END as partner_id "
    "FROM direct_messages "
    "WHERE sender_id = ? OR receiver_id = ? "
    "ORDER BY partner_id";
  
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, user_id) || !stmt.bindLong(1, user_id) || !stmt.bindLong(2, user_id)) {
    LOG_ERROR << "Failed to bind parameters for getConversationPartners";
    return partners;
  }

  if (stmt.executeQuery()) {
    while (stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS) {
      partners.push_back(stmt.getLong(0));
    }
    LOG_INFO << "Found " << partners.size() << " conversation partners for user " << user_id;
  } else {
    LOG_ERROR << "Failed to execute query for getConversationPartners";
  }

  return partners;
}

}  // namespace db
