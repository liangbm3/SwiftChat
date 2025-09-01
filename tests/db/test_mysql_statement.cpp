#include <gtest/gtest.h>

#include "../../src/db/mysql_statement.hpp"

// --- 测试配置 ---
// !!! 警告: 请将以下配置修改为你的测试数据库信息 !!!
// !!! 这个数据库中的 gtest_users 表将被频繁创建和销毁 !!!
const char* DB_HOST = "localhost";
const char* DB_USER = "root";
const char* DB_PASS = "0";
const char* DB_NAME = "test_db";
const unsigned int DB_PORT = 4406;

// 测试装置，用于管理每个测试用例的数据库连接和表结构
class MySQLStatementTest : public ::testing::Test {
 protected:
  MYSQL* mysql_conn = nullptr;

  // 在每个测试开始前执行
  void SetUp() override {
    mysql_conn = mysql_init(nullptr);
    ASSERT_NE(mysql_conn, nullptr) << "mysql_init failed";

    ASSERT_NE(mysql_real_connect(mysql_conn, DB_HOST, DB_USER, DB_PASS, DB_NAME,
                                 DB_PORT, nullptr, 0),
              nullptr)
        << "mysql_real_connect failed: " << mysql_error(mysql_conn);

    // 创建一个干净的测试表
    const char* create_table_sql = R"(
            CREATE TABLE gtest_users (
                id INT AUTO_INCREMENT PRIMARY KEY,
                name VARCHAR(100) NOT NULL,
                age INT,
                balance BIGINT,
                description VARCHAR(255) NULL
            )
        )";
    ASSERT_EQ(mysql_query(mysql_conn, "DROP TABLE IF EXISTS gtest_users"), 0);
    ASSERT_EQ(mysql_query(mysql_conn, create_table_sql), 0)
        << "Failed to create test table: " << mysql_error(mysql_conn);
  }

  // 在每个测试结束后执行
  void TearDown() override {
    if (mysql_conn) {
      mysql_query(mysql_conn, "DROP TABLE IF EXISTS gtest_users");
      mysql_close(mysql_conn);
    }
  }
};

// --- 单元测试 ---

// 测试构造函数能否成功处理一个合法的SQL查询
TEST_F(MySQLStatementTest, ConstructorWithValidQuery) {
  // 这个测试的成功隐含在没有崩溃或错误日志中
  // 并且对象可以被成功创建和销毁
  ASSERT_NO_THROW({
    db::MySQLStatement stmt(mysql_conn,
                            "SELECT id FROM gtest_users WHERE id = ?");
  });
}

// 测试绑定参数到越界索引时应失败
TEST_F(MySQLStatementTest, BindToOutOfRangeIndex) {
  db::MySQLStatement stmt(mysql_conn,
                          "SELECT id FROM gtest_users WHERE id = ?");
  EXPECT_FALSE(stmt.bindInt(1, 123));  // 只有一个占位符，索引是0
  EXPECT_FALSE(stmt.bindString(99, "test"));
}

// --- 集成测试 ---

// 测试完整的 INSERT 和 SELECT 流程
TEST_F(MySQLStatementTest, InsertAndSelect) {
  // 1. 插入数据
  {
    db::MySQLStatement insert_stmt(
        mysql_conn,
        "INSERT INTO gtest_users(name, age, balance) VALUES(?, ?, ?)");
    ASSERT_TRUE(insert_stmt.bindString(0, "Alice"));
    ASSERT_TRUE(insert_stmt.bindInt(1, 30));
    ASSERT_TRUE(insert_stmt.bindLong(2, 10000LL));

    ASSERT_TRUE(insert_stmt.executeUpdate());
    ASSERT_EQ(insert_stmt.getAffectedRows(), 1);
    ASSERT_GT(insert_stmt.getLastInsertId(), 0);
  }

  // 2. 查询并验证数据
  {
    db::MySQLStatement select_stmt(
        mysql_conn,
        "SELECT name, age, balance FROM gtest_users WHERE name = ?");
    ASSERT_TRUE(select_stmt.bindString(0, "Alice"));
    ASSERT_TRUE(select_stmt.executeQuery());

    ASSERT_EQ(select_stmt.fetch(), db::MySQLStatement::FetchStatus::SUCCESS);

    EXPECT_EQ(select_stmt.getString(0), "Alice");
    EXPECT_EQ(select_stmt.getInt(1), 30);
    EXPECT_EQ(select_stmt.getLong(2), 10000LL);

    // 确认没有更多数据了
    ASSERT_EQ(select_stmt.fetch(), db::MySQLStatement::FetchStatus::NO_DATA);
  }
}

// 测试 UPDATE 功能
TEST_F(MySQLStatementTest, UpdateAndVerify) {
  // 先插入一条记录
  long long bob_id;
  {
    db::MySQLStatement insert_stmt(
        mysql_conn, "INSERT INTO gtest_users(name, age) VALUES(?, ?)");
    insert_stmt.bindString(0, "Bob");
    insert_stmt.bindInt(1, 40);
    ASSERT_TRUE(insert_stmt.executeUpdate());
    bob_id = insert_stmt.getLastInsertId();
  }

  // 更新这条记录
  {
    db::MySQLStatement update_stmt(
        mysql_conn, "UPDATE gtest_users SET age = ? WHERE id = ?");
    update_stmt.bindInt(0, 42);
    update_stmt.bindLong(1, bob_id);
    ASSERT_TRUE(update_stmt.executeUpdate());
    ASSERT_EQ(update_stmt.getAffectedRows(), 1);
  }

  // 验证更新结果
  {
    db::MySQLStatement select_stmt(mysql_conn,
                                   "SELECT age FROM gtest_users WHERE id = ?");
    select_stmt.bindLong(0, bob_id);
    ASSERT_TRUE(select_stmt.executeQuery());
    ASSERT_EQ(select_stmt.fetch(), db::MySQLStatement::FetchStatus::SUCCESS);
    EXPECT_EQ(select_stmt.getInt(0), 42);
  }
}

// 测试 DELETE 功能
TEST_F(MySQLStatementTest, DeleteAndVerify) {
  // 先插入一条记录
  long long charlie_id;
  {
    db::MySQLStatement insert_stmt(
        mysql_conn, "INSERT INTO gtest_users(name, age) VALUES(?, ?)");
    insert_stmt.bindString(0, "Charlie");
    insert_stmt.bindInt(1, 25);
    ASSERT_TRUE(insert_stmt.executeUpdate());
    charlie_id = insert_stmt.getLastInsertId();
  }

  // 删除这条记录
  {
    db::MySQLStatement delete_stmt(mysql_conn,
                                   "DELETE FROM gtest_users WHERE id = ?");
    delete_stmt.bindLong(0, charlie_id);
    ASSERT_TRUE(delete_stmt.executeUpdate());
    ASSERT_EQ(delete_stmt.getAffectedRows(), 1);
  }

  // 验证记录已被删除
  {
    db::MySQLStatement select_stmt(mysql_conn,
                                   "SELECT id FROM gtest_users WHERE id = ?");
    select_stmt.bindLong(0, charlie_id);
    ASSERT_TRUE(select_stmt.executeQuery());
    ASSERT_EQ(select_stmt.fetch(), db::MySQLStatement::FetchStatus::NO_DATA);
  }
}

// 测试查询多行结果
TEST_F(MySQLStatementTest, SelectMultipleRows) {
  // 插入3条记录
  {
    db::MySQLStatement insert_stmt(
        mysql_conn, "INSERT INTO gtest_users(name, age) VALUES(?, ?)");
    for (int i = 0; i < 3; ++i) {
      insert_stmt.bindString(0, "User" + std::to_string(i));
      insert_stmt.bindInt(1, 20 + i);
      ASSERT_TRUE(insert_stmt.executeUpdate());
    }
  }

  // 查询并遍历所有结果
  {
    db::MySQLStatement select_stmt(
        mysql_conn, "SELECT name, age FROM gtest_users ORDER BY age");
    ASSERT_TRUE(select_stmt.executeQuery());

    int row_count = 0;
    while (select_stmt.fetch() == db::MySQLStatement::FetchStatus::SUCCESS) {
      EXPECT_EQ(select_stmt.getString(0), "User" + std::to_string(row_count));
      EXPECT_EQ(select_stmt.getInt(1), 20 + row_count);
      row_count++;
    }
    EXPECT_EQ(row_count, 3);
  }
}

// 测试处理 NULL 值
TEST_F(MySQLStatementTest, HandleNullValues) {
  // 插入一条带有 NULL 值的记录
  {
    db::MySQLStatement insert_stmt(
        mysql_conn, "INSERT INTO gtest_users(name, description) VALUES(?, ?)");
    insert_stmt.bindString(0, "David");
    insert_stmt.bindNull(1);
    ASSERT_TRUE(insert_stmt.executeUpdate());
  }

  // 查询并验证 NULL 值
  {
    db::MySQLStatement select_stmt(
        mysql_conn, "SELECT name, description FROM gtest_users WHERE name = ?");
    select_stmt.bindString(0, "David");
    ASSERT_TRUE(select_stmt.executeQuery());

    ASSERT_EQ(select_stmt.fetch(), db::MySQLStatement::FetchStatus::SUCCESS);
    EXPECT_FALSE(select_stmt.isNull(0));  // name 不为 NULL
    EXPECT_TRUE(select_stmt.isNull(1));   // description 应为 NULL
    EXPECT_EQ(select_stmt.getString(1),
              "");  // isNull 为 true 时，getString 返回空字符串
  }
}

// 主函数
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}