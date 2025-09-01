#include "database_initializer.hpp"

#include <vector>

#include "mysql_statement.hpp"

namespace db {

namespace initializer {

bool indexExists(ConnectionPool::ConnPtr conn, const std::string &table_name,
                 const std::string &index_name) {
  try {
    MySQLStatement stmt(
        conn->getRawConnection(),
        "SELECT COUNT(1) FROM INFORMATION_SCHEMA.STATISTICS WHERE table_schema "
        "= DATABASE() AND table_name = ? AND index_name = ?");
    stmt.bindString(0, table_name);
    stmt.bindString(1, index_name);
    if (stmt.executeQuery() &&
        stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS) {
      return stmt.getInt(0) > 0;
    }
  } catch (const std::exception &e) {
    LOG_ERROR << "Error checking index existence: " << e.what();
  }
  return false;
}

bool execute(ConnectionPool::ConnPtr conn, const std::string &query) {
  if (!conn || !conn->isConnected()) {
    LOG_ERROR << "Cannot execute query: no valid connection.";
    return false;
  }
  if (mysql_query(conn->getRawConnection(), query.c_str())) {
    LOG_ERROR << "Query failed: " << mysql_error(conn->getRawConnection())
              << " [SQL: " << query << "]";
    return false;
  }
  return true;
}

bool initializeSchema(ConnectionPool &pool) {
  auto conn = pool.getConnection();
  if (!conn) {
    LOG_ERROR << "Failed to get connection for schema initialization.";
    return false;
  }

  const std::vector<std::string> table_queries = {
      // 1. Users Table
      "CREATE TABLE IF NOT EXISTS users ("
      "id BIGINT PRIMARY KEY AUTO_INCREMENT,"
      "username VARCHAR(50) UNIQUE NOT NULL,"
      "password_hash VARCHAR(255) NOT NULL,"
      "created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),"
      "status TINYINT NOT NULL DEFAULT 0,"
      "last_seen TIMESTAMP(6) NULL DEFAULT NULL"
      ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;",

      // 2. Rooms Table
      "CREATE TABLE IF NOT EXISTS rooms ("
      "id BIGINT PRIMARY KEY AUTO_INCREMENT,"
      "name VARCHAR(100) UNIQUE NOT NULL,"
      "description TEXT NULL,"
      "creator_id BIGINT NOT NULL,"
      "created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),"
      "FOREIGN KEY (creator_id) REFERENCES users(id) ON DELETE CASCADE"
      ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;",

      // 3. Room Members Table
      "CREATE TABLE IF NOT EXISTS room_members ("
      "room_id BIGINT NOT NULL,"
      "user_id BIGINT NOT NULL,"
      "joined_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),"
      "PRIMARY KEY (room_id, user_id),"
      "FOREIGN KEY (room_id) REFERENCES rooms(id) ON DELETE CASCADE,"
      "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
      ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;",

      // 4. Messages Table (Group Messages)
      "CREATE TABLE IF NOT EXISTS messages ("
      "id BIGINT PRIMARY KEY AUTO_INCREMENT,"
      "room_id BIGINT NOT NULL,"
      "sender_id BIGINT NOT NULL,"
      "content TEXT NOT NULL,"
      "created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),"
      "FOREIGN KEY (room_id) REFERENCES rooms(id) ON DELETE CASCADE,"
      "FOREIGN KEY (sender_id) REFERENCES users(id) ON DELETE CASCADE"
      ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;",

      // 5. Direct Messages Table
      "CREATE TABLE IF NOT EXISTS direct_messages ("
      "id BIGINT PRIMARY KEY AUTO_INCREMENT,"
      "sender_id BIGINT NOT NULL,"
      "receiver_id BIGINT NOT NULL,"
      "content TEXT NOT NULL,"
      "created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),"
      "status TINYINT NOT NULL DEFAULT 0,"
      "FOREIGN KEY (sender_id) REFERENCES users(id) ON DELETE CASCADE,"
      "FOREIGN KEY (receiver_id) REFERENCES users(id) ON DELETE CASCADE"
      ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;"};

  // 索引信息
  struct IndexInfo {
    std::string table;
    std::string name;
    std::string definition;
  };

  const std::vector<IndexInfo> index_infos = {
      {"messages", "idx_messages_room_id_created_at",
       "CREATE INDEX idx_messages_room_id_created_at ON messages(room_id, "
       "created_at DESC);"},
      {"direct_messages", "idx_direct_messages_sender_receiver",
       "CREATE INDEX idx_direct_messages_sender_receiver ON "
       "direct_messages(sender_id, receiver_id, created_at DESC);"},
      {"direct_messages", "idx_direct_messages_receiver_sender",
       "CREATE INDEX idx_direct_messages_receiver_sender ON "
       "direct_messages(receiver_id, sender_id, created_at DESC);"}};

  LOG_INFO << "Starting schema initialization...";

  // 创建所有数据表
  LOG_INFO << "Creating tables...";
  for (const auto &query : table_queries) {
    if (!execute(conn, query)) {
      LOG_WARN << "A query failed, possibly because an index already exists, "
                  "which is often safe to ignore.";
      return false;
    }
  }
  LOG_INFO << "Schema initialization finished.";

  // 安全地创建所有索引
  LOG_INFO << "Creating indexes...";
  for (const auto &index_info : index_infos) {
    if (!indexExists(conn, index_info.table, index_info.name)) {
      if (!execute(conn, index_info.definition)) {
        LOG_ERROR << "A query failed, possibly because an index already "
                     "exists, which is often safe to ignore.";
        return false;
      }
    } else {
      LOG_INFO << "Index " << index_info.name << " already exists, skipping.";
    }
  }
  LOG_INFO << "Index creation finished.";
  return true;  // 即使有警告也认为成功
}
}  // namespace initializer

}  // namespace db
