#include "connection_pool.hpp"

namespace db {

ConnectionPool& ConnectionPool::getInstance() {
  static ConnectionPool instance;
  return instance;
}

void ConnectionPool::init(const MySQLConfig& config, size_t pool_size) {
  config_ = config;
  pool_size_ = pool_size;

  std::lock_guard<std::mutex> lock(mutex_);
  for (size_t i = 0; i < pool_size_; ++i) {
    auto conn = std::make_unique<DatabaseConnection>(config_);
    if (conn->connect()) {
      connection_queue_.push(conn.get());
      all_connections_.push_back(std::move(conn));
    } else {
      LOG_ERROR << "Failed to establish database connection " << (i + 1) << "/"
                << pool_size_;
    }
  }
  LOG_INFO << "Connection pool initialized with " << connection_queue_.size()
           << " connections.";
}

ConnectionPool::~ConnectionPool() {
  // allConnections_ 的 unique_ptr 会自动释放所有 DatabaseConnection 对象
  // 这将调用 DatabaseConnection 的析构函数，关闭所有 mysql 连接
  LOG_INFO << "Connection pool is being destroyed.";
}

ConnectionPool::ConnPtr ConnectionPool::getConnection() {
  std::unique_lock<std::mutex> lock(mutex_);

  // 最多等待两秒
  cond_.wait_for(lock, std::chrono::seconds(2),
                 [this]() { return !connection_queue_.empty(); });

  DatabaseConnection* raw_conn = connection_queue_.front();
  connection_queue_.pop();
  // 如果连接断开或者ping不通，则尝试重连
  if (mysql_ping(raw_conn->getRawConnection()) != 0) {
    LOG_WARN << "Database connection lost. Attempting to reconnect...";
    if (!raw_conn->reconnect()) {
      LOG_ERROR << "Reconnection failed.";
    } else {
      LOG_INFO << "Reconnected to the database successfully.";
    }
  }

  // 使用自定义删除器，当share_ptr销毁时，自动将连接归还到池中
  return ConnPtr(raw_conn,
                 [this](DatabaseConnection* conn) { returnConnection(conn); });
}

void ConnectionPool::returnConnection(DatabaseConnection* conn) {
  if (conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    connection_queue_.push(conn);
    cond_.notify_one();
  }
}

}  // namespace db
