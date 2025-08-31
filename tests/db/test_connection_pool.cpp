// run_pool_tests.cpp
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "../src/db/connection_pool.hpp"
#include "../src/db/mysql_statement.hpp"

// --- 测试配置 ---
// !!! 警告: 请将以下配置修改为你的测试数据库信息 !!!
db::MySQLConfig DB_CONFIG = {"localhost", 4406, "test_db", "root", "0"};

// --- 单元测试: DatabaseConnection ---

// 测试能否使用有效配置成功连接
TEST(DatabaseConnectionTest, ConnectSuccessfully) {
  db::DatabaseConnection conn(DB_CONFIG);
  ASSERT_TRUE(conn.connect());
  EXPECT_TRUE(conn.isConnected());
}

// 测试使用无效配置（错误密码）时连接失败
TEST(DatabaseConnectionTest, ConnectWithInvalidPasswordFails) {
  db::MySQLConfig bad_config = DB_CONFIG;
  bad_config.password = "wrong_password";

  db::DatabaseConnection conn(bad_config);
  ASSERT_FALSE(conn.connect());
  EXPECT_FALSE(conn.isConnected());
}

// --- 集成与并发测试: ConnectionPool ---

class ConnectionPoolTest : public ::testing::Test {
 protected:
  // 每个测试用例都在一个干净的单例上运行
  // 注意：由于是单例，我们不能在每个测试中重新构造它，
  // 所以我们将测试逻辑组织好，使其可以按顺序工作。
};

// 测试单例模式是否正常工作
TEST_F(ConnectionPoolTest, SingletonBehavesCorrectly) {
  db::ConnectionPool& pool1 = db::ConnectionPool::getInstance();
  db::ConnectionPool& pool2 = db::ConnectionPool::getInstance();
  ASSERT_EQ(&pool1, &pool2);
}

// 测试连接池的初始化、获取和自动归还
TEST_F(ConnectionPoolTest, InitGetAndAutoReturn) {
  const size_t POOL_SIZE = 3;
  db::ConnectionPool& pool = db::ConnectionPool::getInstance();

  // 初始化
  pool.init(DB_CONFIG, POOL_SIZE);

  std::vector<db::ConnectionPool::ConnPtr> connections;
  // 1. 获取所有连接
  for (size_t i = 0; i < POOL_SIZE; ++i) {
    auto conn = pool.getConnection();
    ASSERT_NE(conn, nullptr);
    ASSERT_TRUE(conn->isConnected());
    connections.push_back(std::move(conn));
  }

  // 此刻池应该是空的
  // 2. 归还一个连接
  // 当 conn1 离开作用域，它管理的连接应该被自动归还
  {
    auto conn1 = std::move(connections.back());
    connections.pop_back();
  }  // conn1 在这里被销毁，连接被归还

  // 3. 应该能立即获取到一个新的连接
  auto new_conn = pool.getConnection();
  ASSERT_NE(new_conn, nullptr);
  ASSERT_TRUE(new_conn->isConnected());
}

// 关键测试: 多线程压力测试
TEST_F(ConnectionPoolTest, MultiThreadedStressTest) {
  const size_t POOL_SIZE = 8;
  const int NUM_THREADS = 20;  // 使用比连接池更多的线程来制造争抢
  const int OPS_PER_THREAD = 50;

  db::ConnectionPool& pool = db::ConnectionPool::getInstance();
  pool.init(DB_CONFIG, POOL_SIZE);

  std::vector<std::thread> threads;
  std::atomic<int> successful_ops = 0;
  std::atomic<bool> test_failed = false;

  auto worker_task = [&]() {
    for (int i = 0; i < OPS_PER_THREAD; ++i) {
      try {
        auto conn = pool.getConnection();
        if (!conn) {
          // 如果getConnection实现了超时，可能会返回nullptr
          LOG_ERROR << "Thread " << std::this_thread::get_id()
                    << " failed to get connection.";
          test_failed = true;
          continue;
        }

        // 使用连接执行一个简单的查询
        db::MySQLStatement stmt(conn->getRawConnection(), "SELECT 1");
        if (stmt.executeQuery()) {
          if (stmt.fetch() == db::MySQLStatement::FetchStatus::SUCCESS) {
            if (stmt.getInt(0) == 1) {
              successful_ops++;
            } else {
              test_failed = true;
            }
          } else {
            test_failed = true;
          }
        } else {
          test_failed = true;
        }
        // conn 离开作用域时，连接会自动归还
      } catch (const std::exception& e) {
        LOG_ERROR << "Exception in thread " << std::this_thread::get_id()
                  << ": " << e.what();
        test_failed = true;
      }
    }
  };

  // 创建并启动所有线程
  for (int i = 0; i < NUM_THREADS; ++i) {
    threads.emplace_back(worker_task);
  }

  // 等待所有线程完成
  for (auto& t : threads) {
    t.join();
  }

  // 验证结果
  ASSERT_FALSE(test_failed);
  EXPECT_EQ(successful_ops, NUM_THREADS * OPS_PER_THREAD);
}

// 主函数
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}