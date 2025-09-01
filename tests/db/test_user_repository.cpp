#include <gtest/gtest.h>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>

#include "../../src/db/respository/user_repository.hpp"
#include "../../src/db/connection_pool.hpp"
#include "../../src/db/database_initializer.hpp"
#include "../../src/utils/logger.hpp"

using namespace db;

class UserRepositoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 设置测试数据库连接
        MySQLConfig config;
        config.host = "localhost";
        config.port = 4406;
        config.username = "root";
        config.password = "0";
        config.database = "test_db";
        
        auto& pool_ref = ConnectionPool::getInstance();
        pool = &pool_ref;
        pool->init(config, 5);
        
        // 初始化数据库架构
        ASSERT_TRUE(initializer::initializeSchema(*pool)) << "Failed to initialize test database schema";
        
        // 创建 UserRepository 实例
        user_repo = std::make_unique<UserRepository>(*pool);
        
        // 清理测试数据
        cleanupTestData();
    }

    void TearDown() override {
        // 清理测试数据
        cleanupTestData();
    }

    void cleanupTestData() {
        auto conn = ConnectionPool::getInstance().getConnection();
        if (conn) {
            mysql_query(conn->getRawConnection(), "DELETE FROM users WHERE username LIKE 'test_%'");
        }
    }

    ConnectionPool* pool;
    std::unique_ptr<UserRepository> user_repo;
};

// 测试用户创建功能
TEST_F(UserRepositoryTest, CreateUser) {
    std::string username = "test_user_create";
    std::string password_hash = "hashed_password_123";
    
    // 测试成功创建用户
    auto user_id = user_repo->createUser(username, password_hash);
    ASSERT_TRUE(user_id.has_value()) << "Failed to create user";
    EXPECT_GT(user_id.value(), 0) << "User ID should be positive";
    
    // 测试重复用户名创建失败
    auto duplicate_id = user_repo->createUser(username, "different_password");
    EXPECT_FALSE(duplicate_id.has_value()) << "Should not allow duplicate usernames";
}

// 测试用户存在性检查
TEST_F(UserRepositoryTest, UserExists) {
    std::string username = "test_user_exists";
    std::string password_hash = "hashed_password_456";
    
    // 创建测试用户
    auto user_id = user_repo->createUser(username, password_hash);
    ASSERT_TRUE(user_id.has_value()) << "Failed to create test user";
    
    // 测试通过ID检查用户存在
    EXPECT_TRUE(user_repo->userExists(user_id.value())) << "User should exist by ID";
    EXPECT_FALSE(user_repo->userExists(99999)) << "Non-existent user should not exist";
    
    // 测试通过用户名检查用户存在
    EXPECT_TRUE(user_repo->userExists(username)) << "User should exist by username";
    EXPECT_FALSE(user_repo->userExists("non_existent_user")) << "Non-existent user should not exist";
}

// 测试用户验证功能
TEST_F(UserRepositoryTest, ValidateUser) {
    std::string username = "test_user_validate";
    std::string password_hash = "hashed_password_789";
    
    // 创建测试用户
    auto user_id = user_repo->createUser(username, password_hash);
    ASSERT_TRUE(user_id.has_value()) << "Failed to create test user";
    
    // 测试正确凭据验证
    EXPECT_TRUE(user_repo->validateUser(username, password_hash)) << "Valid credentials should pass";
    
    // 测试错误密码验证
    EXPECT_FALSE(user_repo->validateUser(username, "wrong_password")) << "Wrong password should fail";
    
    // 测试不存在的用户名验证
    EXPECT_FALSE(user_repo->validateUser("non_existent", password_hash)) << "Non-existent user should fail";
}

// 测试用户状态更新
TEST_F(UserRepositoryTest, UpdateUserStatus) {
    std::string username = "test_user_status";
    std::string password_hash = "hashed_password_abc";
    
    // 创建测试用户
    auto user_id = user_repo->createUser(username, password_hash);
    ASSERT_TRUE(user_id.has_value()) << "Failed to create test user";
    
    // 测试更新用户状态为在线
    EXPECT_TRUE(user_repo->updateUserStatus(user_id.value(), 1)) << "Should update status to online";
    
    // 验证状态更新
    auto user = user_repo->getUser(user_id.value());
    ASSERT_TRUE(user.has_value()) << "Should retrieve user";
    EXPECT_EQ(user->getStatus(), 1) << "Status should be online (1)";
    
    // 测试更新用户状态为离线
    EXPECT_TRUE(user_repo->updateUserStatus(user_id.value(), 0)) << "Should update status to offline";
    
    // 验证状态更新
    user = user_repo->getUser(user_id.value());
    ASSERT_TRUE(user.has_value()) << "Should retrieve user";
    EXPECT_EQ(user->getStatus(), 0) << "Status should be offline (0)";
    
    // 测试更新不存在用户的状态
    EXPECT_FALSE(user_repo->updateUserStatus(99999, 1)) << "Should fail for non-existent user";
}

// 测试最后在线时间更新
TEST_F(UserRepositoryTest, UpdateLastSeen) {
    std::string username = "test_user_lastseen";
    std::string password_hash = "hashed_password_def";
    
    // 创建测试用户
    auto user_id = user_repo->createUser(username, password_hash);
    ASSERT_TRUE(user_id.has_value()) << "Failed to create test user";
    
    // 更新最后在线时间
    EXPECT_TRUE(user_repo->updateLastSeen(user_id.value())) << "Should update last seen time";
    
    // 验证时间更新
    auto user = user_repo->getUser(user_id.value());
    ASSERT_TRUE(user.has_value()) << "Should retrieve user";
    EXPECT_FALSE(user->getLastSeen().empty()) << "Last seen should not be empty after update";
    
    // 测试更新不存在用户的最后在线时间
    EXPECT_FALSE(user_repo->updateLastSeen(99999)) << "Should fail for non-existent user";
}

// 测试用户查询功能
TEST_F(UserRepositoryTest, GetUser) {
    std::string username = "test_user_get";
    std::string password_hash = "hashed_password_ghi";
    
    // 创建测试用户
    auto user_id = user_repo->createUser(username, password_hash);
    ASSERT_TRUE(user_id.has_value()) << "Failed to create test user";
    
    // 测试通过ID获取用户
    auto user_by_id = user_repo->getUser(user_id.value());
    ASSERT_TRUE(user_by_id.has_value()) << "Should retrieve user by ID";
    EXPECT_EQ(user_by_id->getId(), user_id.value()) << "User ID should match";
    EXPECT_EQ(user_by_id->getUsername(), username) << "Username should match";
    EXPECT_EQ(user_by_id->getPassword(), password_hash) << "Password hash should match";
    
    // 测试通过用户名获取用户
    auto user_by_username = user_repo->getUser(username);
    ASSERT_TRUE(user_by_username.has_value()) << "Should retrieve user by username";
    EXPECT_EQ(user_by_username->getId(), user_id.value()) << "User ID should match";
    EXPECT_EQ(user_by_username->getUsername(), username) << "Username should match";
    EXPECT_EQ(user_by_username->getPassword(), password_hash) << "Password hash should match";
    
    // 测试获取不存在的用户
    auto non_existent_by_id = user_repo->getUser(99999);
    EXPECT_FALSE(non_existent_by_id.has_value()) << "Non-existent user by ID should return nullopt";
    
    auto non_existent_by_username = user_repo->getUser("non_existent_user");
    EXPECT_FALSE(non_existent_by_username.has_value()) << "Non-existent user by username should return nullopt";
}

// 测试获取所有用户
TEST_F(UserRepositoryTest, GetAllUsers) {
    // 创建多个测试用户
    std::vector<std::string> usernames = {"test_user_all_1", "test_user_all_2", "test_user_all_3"};
    std::vector<int64_t> user_ids;
    
    for (const auto& username : usernames) {
        auto user_id = user_repo->createUser(username, "password_" + username);
        ASSERT_TRUE(user_id.has_value()) << "Failed to create user: " << username;
        user_ids.push_back(user_id.value());
    }
    
    // 获取所有用户
    auto all_users = user_repo->getAllUsers();
    
    // 验证包含我们创建的用户
    EXPECT_GE(all_users.size(), usernames.size()) << "Should contain at least our test users";
    
    // 检查我们的测试用户是否在结果中
    for (const auto& username : usernames) {
        bool found = false;
        for (const auto& user : all_users) {
            if (user.getUsername() == username) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "User " << username << " should be in all users list";
    }
}

// 测试获取在线用户
TEST_F(UserRepositoryTest, GetOnlineUsers) {
    // 创建多个测试用户，部分设为在线
    std::vector<std::pair<std::string, int>> users_status = {
        {"test_online_1", 1},  // 在线
        {"test_offline_1", 0}, // 离线
        {"test_online_2", 1},  // 在线
        {"test_offline_2", 0}  // 离线
    };
    
    std::vector<int64_t> online_user_ids;
    
    for (const auto& [username, status] : users_status) {
        auto user_id = user_repo->createUser(username, "password_" + username);
        ASSERT_TRUE(user_id.has_value()) << "Failed to create user: " << username;
        
        // 设置用户状态
        ASSERT_TRUE(user_repo->updateUserStatus(user_id.value(), status)) 
            << "Failed to update status for user: " << username;
        
        if (status == 1) {
            online_user_ids.push_back(user_id.value());
            // 更新最后在线时间
            user_repo->updateLastSeen(user_id.value());
        }
    }
    
    // 获取在线用户
    auto online_users = user_repo->getOnlineUsers();
    
    // 验证在线用户数量
    int expected_online_count = 0;
    for (const auto& [username, status] : users_status) {
        if (status == 1) expected_online_count++;
    }
    
    // 检查至少包含我们的在线用户
    int found_online_count = 0;
    for (const auto& user : online_users) {
        if (user.getUsername().find("test_online_") == 0) {
            found_online_count++;
            EXPECT_EQ(user.getStatus(), 1) << "Online user should have status 1";
        }
    }
    
    EXPECT_EQ(found_online_count, expected_online_count) 
        << "Should find exactly " << expected_online_count << " online test users";
    
    // 验证排序（按 last_seen DESC）
    if (online_users.size() > 1) {
        for (size_t i = 1; i < online_users.size(); ++i) {
            EXPECT_GE(online_users[i-1].getLastSeen(), online_users[i].getLastSeen())
                << "Online users should be sorted by last_seen DESC";
        }
    }
}

// 测试边界情况和错误处理
TEST_F(UserRepositoryTest, EdgeCasesAndErrorHandling) {
    // 测试空用户名
    auto empty_username_result = user_repo->createUser("", "password");
    // 这可能成功也可能失败，取决于数据库约束
    
    // 测试非常长的用户名（假设数据库有长度限制）
    std::string long_username(1000, 'a');
    auto long_username_result = user_repo->createUser(long_username, "password");
    // 这应该失败或被截断
    
    // 测试空密码
    auto empty_password_result = user_repo->createUser("test_empty_pass", "");
    // 这可能成功也可能失败，取决于业务逻辑
}

// 测试多线程安全性（简单测试）
TEST_F(UserRepositoryTest, ConcurrentAccess) {
    const int num_threads = 5;
    const int users_per_thread = 10;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, users_per_thread, &success_count]() {
            for (int i = 0; i < users_per_thread; ++i) {
                std::string username = "test_concurrent_" + std::to_string(t) + "_" + std::to_string(i);
                auto user_id = user_repo->createUser(username, "password_" + username);
                if (user_id.has_value()) {
                    success_count++;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(success_count.load(), num_threads * users_per_thread) 
        << "All concurrent user creations should succeed";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // 初始化日志系统
    utils::Logger::setGlobalLevel(utils::LogLevel::INFO);
    
    return RUN_ALL_TESTS();
}
