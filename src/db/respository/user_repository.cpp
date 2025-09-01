#include "user_repository.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>


namespace db {
UserRepository::UserRepository(ConnectionPool &pool) : pool_(pool) {}

std::optional<int64_t> UserRepository::createUser(const std::string &username,
                                                  const std::string &password_hash) {
  auto connection = pool_.getConnection();
  if (!connection) {
    return std::nullopt;
  }

  // 检查用户名是否已存在
  if (userExists(username)) {
    return std::nullopt;
  }

  const char *sql = "INSERT INTO users (username, password_hash, created_at) VALUES (?, ?, NOW())";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindString(0, username) || !stmt.bindString(1, password_hash)) {
    return std::nullopt;
  }

  if(stmt.executeUpdate()&&stmt.getAffectedRows()==1){
    long long user_id = stmt.getLastInsertId();
    LOG_INFO<<"User '"<<username<<"' created with ID: "<<user_id;
    return user_id;
  }
  LOG_ERROR << "Failed to create user '" << username << "'";
  return std::nullopt;
}

bool UserRepository::deleteUser(int64_t user_id) {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return false;
  }

  const char *sql = "DELETE FROM users WHERE id = ?";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, user_id)) {
    LOG_ERROR << "Failed to bind parameters for deleteUser";
    return false;
  }

  if (stmt.executeUpdate() && stmt.getAffectedRows() >= 1) {
    LOG_INFO << "User with ID " << user_id << " deleted successfully";
    return true;
  }

  LOG_ERROR << "Failed to delete user with ID: " << user_id;
  return false;
}

bool UserRepository::validateUser(const std::string &username,
                                  const std::string &password_hash) {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection for validateUser";
    return false;
  }

  const char *sql = "SELECT COUNT(*) FROM users WHERE username = ? AND password_hash = ?";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindString(0, username) || !stmt.bindString(1, password_hash)) {
    LOG_ERROR << "Failed to bind parameters for validateUser";
    return false;
  }

  if (stmt.executeQuery() && stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS) {
    return stmt.getInt(0) > 0;
  }

  LOG_ERROR << "Failed to validate user '" << username << "'";
  return false;
}

bool UserRepository::userExists(int64_t user_id) {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return false;
  }

  const char *sql = "SELECT COUNT(*) FROM users WHERE id = ?";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, user_id)) {
    LOG_ERROR << "Failed to bind user ID parameter";
    return false;
  }

  if(stmt.executeQuery()&&stmt.fetch()==MySQLStatement::FetchStatus::SUCCESS){
    return stmt.getInt(0) > 0;
  }
  LOG_ERROR << "Failed to check if user exists with ID: " << user_id;
  return false;
}

bool UserRepository::userExists(const std::string &username) {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return false;
  }

  const char *sql = "SELECT COUNT(*) FROM users WHERE username = ?";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindString(0, username)) {
    LOG_ERROR << "Failed to bind username parameter";
    return false;
  }

  if (!stmt.executeQuery()) {
    LOG_ERROR << "Failed to execute query for userExists(username)";
    return false;
  }

  if (stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS) {
    return stmt.getInt(0) > 0;
  }

  LOG_ERROR << "Failed to check if user exists with username: " << username;
  return false;
}

bool UserRepository::updateUserStatus(int64_t user_id, int status) {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return false;
  }

  // 先检查用户是否存在
  const char *check_sql = "SELECT id FROM users WHERE id = ?";
  MySQLStatement check_stmt(connection->getRawConnection(), check_sql);
  
  if (!check_stmt.bindLong(0, user_id)) {
    LOG_ERROR << "Failed to bind user ID parameter for existence check";
    return false;
  }
  
  bool user_exists = false;
  if (check_stmt.executeQuery()) {
    user_exists = (check_stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS);
  } else {
    LOG_ERROR << "Failed to execute existence check query for user ID: " << user_id;
    return false;
  }
  
  if (!user_exists) {
    LOG_ERROR << "User with ID " << user_id << " does not exist";
    return false;
  }

  // 用户存在，执行更新
  const char *sql = "UPDATE users SET status = ? WHERE id = ?";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindInt(0, status) || !stmt.bindLong(1, user_id)) {
    LOG_ERROR << "Failed to bind parameters for updateUserStatus";
    return false;
  }

  if (stmt.executeUpdate()) {
    return true;  // 更新成功，无论是否有行被影响
  }
  LOG_ERROR << "Failed to update user status for ID: " << user_id;
  return false;
}

bool UserRepository::updateLastSeen(int64_t user_id) {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return false;
  }

  const char *sql = "UPDATE users SET last_seen = NOW() WHERE id = ?";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, user_id)) {
    LOG_ERROR << "Failed to bind parameters for updateLastSeen";
    return false;
  }

  if (stmt.executeUpdate() && stmt.getAffectedRows() == 1) {
    return true;
  }

  LOG_ERROR << "Failed to update last seen for user ID: " << user_id;
  return false;
}

std::vector<User> UserRepository::getAllUsers() const {
  std::vector<User> users;
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection";
    return users;
  }

  const char *sql = "SELECT id, username, password_hash, status, last_seen FROM users";

  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (stmt.executeQuery()) {
    while(stmt.fetch()==MySQLStatement::FetchStatus::SUCCESS) {
      int64_t id = stmt.getLong(0);
      std::string username = stmt.getString(1);
      std::string password = stmt.getString(2);
      int status = stmt.getInt(3);
      std::string last_seen = stmt.getString(4);

      users.emplace_back(id, username, password, status, last_seen);
    }
  }
  else {
    LOG_ERROR << "Failed to execute query: " << sql;
  }
  return users;
}

std::optional<User> UserRepository::getUser(int64_t user_id) const {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection for getUser(id)";
    return std::nullopt;
  }

  const char *sql = "SELECT id, username, password_hash, status, last_seen FROM users WHERE id = ?";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindLong(0, user_id)) {
    LOG_ERROR << "Failed to bind user ID parameter for getUser";
    return std::nullopt;
  }

  if (stmt.executeQuery() && stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS) {
    int64_t id = stmt.getLong(0);
    std::string username = stmt.getString(1);
    std::string password_hash = stmt.getString(2);
    int status = stmt.getInt(3);
    std::string last_seen = stmt.getString(4);

    return User(id, username, password_hash, status, last_seen);
  }

  LOG_ERROR << "Failed to find user with ID: " << user_id;
  return std::nullopt;
}

std::optional<User> UserRepository::getUser(const std::string &username) const {
  auto connection = pool_.getConnection();
  if (!connection) {
    LOG_ERROR << "Failed to get database connection for getUser(username)";
    return std::nullopt;
  }

  const char *sql = "SELECT id, username, password_hash, status, last_seen FROM users WHERE username = ?";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (!stmt.bindString(0, username)) {
    LOG_ERROR << "Failed to bind username parameter for getUser";
    return std::nullopt;
  }

  if (stmt.executeQuery() && stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS) {
    int64_t id = stmt.getLong(0);
    std::string username = stmt.getString(1);
    std::string password_hash = stmt.getString(2);
    int status = stmt.getInt(3);
    std::string last_seen = stmt.getString(4);

    return User(id, username, password_hash, status, last_seen);
  }

  LOG_ERROR << "Failed to find user with username: " << username;
  return std::nullopt;
}

std::vector<User> UserRepository::getOnlineUsers() const {
  auto connection = pool_.getConnection();
  std::vector<User> users;
  
  if (!connection) {
    LOG_ERROR << "Failed to get connection for getOnlineUsers";
    return users;
  }

  const char *sql = "SELECT id, username, password_hash, status, last_seen FROM users WHERE status = 1 ORDER BY last_seen DESC";
  MySQLStatement stmt(connection->getRawConnection(), sql);

  if (stmt.executeQuery()) {
    while (stmt.fetch() == MySQLStatement::FetchStatus::SUCCESS) {
      int64_t id = stmt.getLong(0);
      std::string username = stmt.getString(1);
      std::string password_hash = stmt.getString(2);
      int status = stmt.getInt(3);
      std::string last_seen = stmt.getString(4);
      
      users.emplace_back(id, username, password_hash, status, last_seen);
    }
    LOG_INFO << "Retrieved " << users.size() << " online users";
  } else {
    LOG_ERROR << "Failed to execute query for getOnlineUsers";
  }
  
  return users;
}

}  // namespace db
