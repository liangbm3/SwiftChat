#pragma once

#include <mysql/mysql.h>

#include <mutex>
#include <string>

#include "utils/logger.hpp"

namespace db {

// MySQL 连接配置结构
struct MySQLConfig {
  std::string host = "localhost";
  unsigned int port = 4406;
  std::string database = "swiftchat";
  std::string username = "root";
  std::string password = "";
};

// 一个数据库连接
class DatabaseConnection {
 public:
  explicit DatabaseConnection(const MySQLConfig& config);
  virtual ~DatabaseConnection();

  // 禁止拷贝和移动，每个实例管理唯一的连接资源
  DatabaseConnection(const DatabaseConnection&) = delete;
  DatabaseConnection& operator=(const DatabaseConnection&) = delete;
  DatabaseConnection(DatabaseConnection&&) = delete;
  DatabaseConnection& operator=(DatabaseConnection&&) = delete;

  bool connect();
  bool reconnect();
  void disconnect();
  bool isConnected() const { return is_connected_; }
  MYSQL* getRawConnection() const { return mysql_; }

 protected:
  MYSQL* mysql_;        // 指向MySQL 结构体的指针
  MySQLConfig config_;  // MySQL 连接配置
  bool is_connected_;
};

}  // namespace db