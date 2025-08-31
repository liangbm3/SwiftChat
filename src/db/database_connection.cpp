#include "database_connection.hpp"

#include <chrono>

namespace db {

DatabaseConnection::DatabaseConnection(const MySQLConfig &config)
    : config_(config), mysql_(nullptr), is_connected_(false) {
  // 初始化MySQL
  mysql_ = mysql_init(nullptr);
  if (!mysql_) {
    LOG_ERROR << "Failed to initialize MySQL";
    return;
  }
}

DatabaseConnection::~DatabaseConnection() {
  disconnect();
  if (mysql_) {
    LOG_INFO << "Closing MySQL connection";
    mysql_close(mysql_);
  }
}

bool DatabaseConnection::connect() {
  if (is_connected_) {
    return true;
  }
  if (!mysql_) {
    LOG_ERROR << "MySQL connection is null";
    return false;
  }

  // 设置字符集
  if (mysql_options(mysql_, MYSQL_SET_CHARSET_NAME, "utf8mb4")) {
    LOG_ERROR << "Failed to set charset option: " << mysql_error(mysql_);
    return false;
  }

  // 设置自动重连选项
  bool reconnect_flag = 1;
  if (mysql_options(mysql_, MYSQL_OPT_RECONNECT, &reconnect_flag)) {
    LOG_ERROR << "Failed to set reconnect option: " << mysql_error(mysql_);
    return false;
  }

  // 连接到MySQL服务器
  if (!mysql_real_connect(mysql_, config_.host.c_str(),
                          config_.username.c_str(), config_.password.c_str(),
                          config_.database.c_str(), config_.port, nullptr, 0)) {
    LOG_ERROR << "Can't connect to MySQL server: " << mysql_error(mysql_);
    return false;
  }
  
  // 设置自动提交
  if (mysql_autocommit(mysql_, 1)) {
    LOG_ERROR << "Failed to set autocommit: " << mysql_error(mysql_);
    return false;
  }
  
  LOG_INFO << "Connected to MySQL server successfully with utf8mb4 charset";
  is_connected_ = true;
  return true;
}

void DatabaseConnection::disconnect() {
  if (is_connected_) {
    is_connected_ = false;
    LOG_INFO << "MySQL connection is now disconnected";
  }
}

bool DatabaseConnection::reconnect() {
  LOG_INFO << "Reconnecting to MySQL server...";
  disconnect();
  return connect();
}

}  // namespace db