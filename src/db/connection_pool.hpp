#pragma once
#include <condition_variable>
#include <memory>
#include <queue>
#include <vector>

#include "database_connection.hpp"

namespace db {

class ConnectionPool {
 public:
  using ConnPtr = std::shared_ptr<DatabaseConnection>;

  // 获取连接池单例
  static ConnectionPool& getInstance();

  // 初始化连接池
  void init(const MySQLConfig& config, size_t pool_size);

  // 从池中获取一个连接
  ConnPtr getConnection();

 private:
  ConnectionPool() = default;
  ~ConnectionPool();

  // 禁止拷贝构造和赋值
  ConnectionPool(const ConnectionPool&) = delete;
  ConnectionPool& operator=(const ConnectionPool&) = delete;

  // 将连接归还到池中
  void returnConnection(DatabaseConnection* conn);

  MySQLConfig config_;
  int pool_size_;
  std::queue<DatabaseConnection*> connection_queue_;
  std::mutex mutex_;
  std::condition_variable cond_;
  std::vector<std::unique_ptr<DatabaseConnection>> all_connections_;
};

}  // namespace db