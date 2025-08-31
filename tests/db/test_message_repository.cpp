#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>
#include <set>

#include "../../src/db/respository/message_repository.hpp"
#include "../../src/db/respository/user_repository.hpp"
#include "../../src/db/respository/room_repository.hpp"
#include "../../src/db/connection_pool.hpp"
#include "../../src/db/database_initializer.hpp"

using namespace db;

class MessageRepositoryTest : public ::testing::Test {
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
        
        // 创建Repository实例
        message_repo = std::make_unique<MessageRepository>(*pool);
        user_repo = std::make_unique<UserRepository>(*pool);
        room_repo = std::make_unique<RoomRepository>(*pool);
        
        // 清理测试数据
        cleanupTestData();
        
        // 创建测试用户
        setupTestUsers();
        
        // 创建测试房间
        setupTestRooms();
    }

    void TearDown() override {
        cleanupTestData();
    }

    void cleanupTestData() {
        auto conn = ConnectionPool::getInstance().getConnection();
        if (conn) {
            mysql_query(conn->getRawConnection(), "DELETE FROM messages WHERE 1=1");
            mysql_query(conn->getRawConnection(), "DELETE FROM direct_messages WHERE 1=1");
            mysql_query(conn->getRawConnection(), "DELETE FROM room_members WHERE 1=1");
            mysql_query(conn->getRawConnection(), "DELETE FROM rooms WHERE 1=1");
            mysql_query(conn->getRawConnection(), "DELETE FROM users WHERE 1=1");
        }
    }

    void setupTestUsers() {
        // 创建测试用户
        test_user1_id = user_repo->createUser("testuser1", "password123").value_or(0);
        test_user2_id = user_repo->createUser("testuser2", "password456").value_or(0);
        test_user3_id = user_repo->createUser("testuser3", "password789").value_or(0);
        
        ASSERT_GT(test_user1_id, 0);
        ASSERT_GT(test_user2_id, 0);
        ASSERT_GT(test_user3_id, 0);
    }

    void setupTestRooms() {
        // 创建测试房间
        test_room1_id = room_repo->createRoom("Test Room 1", test_user1_id).value_or(0);
        test_room2_id = room_repo->createRoom("Test Room 2", test_user2_id).value_or(0);
        
        ASSERT_GT(test_room1_id, 0);
        ASSERT_GT(test_room2_id, 0);
        
        // 将用户加入房间
        ASSERT_TRUE(room_repo->addRoomMember(test_room1_id, test_user1_id));
        ASSERT_TRUE(room_repo->addRoomMember(test_room1_id, test_user2_id));
        ASSERT_TRUE(room_repo->addRoomMember(test_room2_id, test_user2_id));
        ASSERT_TRUE(room_repo->addRoomMember(test_room2_id, test_user3_id));
    }

    ConnectionPool* pool;
    std::unique_ptr<MessageRepository> message_repo;
    std::unique_ptr<UserRepository> user_repo;
    std::unique_ptr<RoomRepository> room_repo;
    
    int64_t test_user1_id = 0;
    int64_t test_user2_id = 0;
    int64_t test_user3_id = 0;
    int64_t test_room1_id = 0;
    int64_t test_room2_id = 0;
};

// ================== 房间消息测试 ==================

TEST_F(MessageRepositoryTest, SaveMessage_ValidData_Success) {
    const std::string content = "Hello, this is a test message!";
    
    auto message_id = message_repo->saveMessage(test_room1_id, test_user1_id, content);
    
    ASSERT_TRUE(message_id.has_value());
    EXPECT_GT(message_id.value(), 0);
    
    // 验证消息存在
    EXPECT_TRUE(message_repo->messageExists(message_id.value()));
}

TEST_F(MessageRepositoryTest, SaveMessage_InvalidRoom_Failure) {
    const int64_t invalid_room_id = 99999;
    const std::string content = "This should fail";
    
    auto message_id = message_repo->saveMessage(invalid_room_id, test_user1_id, content);
    
    // 由于外键约束，这应该失败
    EXPECT_FALSE(message_id.has_value());
}

TEST_F(MessageRepositoryTest, SaveMessage_EmptyContent_Success) {
    const std::string empty_content = "";
    
    auto message_id = message_repo->saveMessage(test_room1_id, test_user1_id, empty_content);
    
    ASSERT_TRUE(message_id.has_value());
    EXPECT_GT(message_id.value(), 0);
}

TEST_F(MessageRepositoryTest, DeleteMessage_ExistingMessage_Success) {
    // 首先创建一条消息
    auto message_id = message_repo->saveMessage(test_room1_id, test_user1_id, "Message to delete");
    ASSERT_TRUE(message_id.has_value());
    
    // 删除消息
    bool deleted = message_repo->deleteMessage(message_id.value());
    EXPECT_TRUE(deleted);
    
    // 验证消息不再存在
    EXPECT_FALSE(message_repo->messageExists(message_id.value()));
}

TEST_F(MessageRepositoryTest, DeleteMessage_NonExistentMessage_Failure) {
    const int64_t non_existent_id = 99999;
    
    bool deleted = message_repo->deleteMessage(non_existent_id);
    EXPECT_FALSE(deleted);
}

TEST_F(MessageRepositoryTest, GetRoomMessages_ValidRoom_Success) {
    // 创建几条测试消息
    auto msg1_id = message_repo->saveMessage(test_room1_id, test_user1_id, "First message");
    auto msg2_id = message_repo->saveMessage(test_room1_id, test_user2_id, "Second message");
    auto msg3_id = message_repo->saveMessage(test_room1_id, test_user1_id, "Third message");
    
    ASSERT_TRUE(msg1_id.has_value());
    ASSERT_TRUE(msg2_id.has_value());
    ASSERT_TRUE(msg3_id.has_value());
    
    // 获取房间消息
    auto messages = message_repo->getRoomMessages(test_room1_id, 10, 0);
    
    EXPECT_EQ(messages.size(), 3);
    
    // 验证消息按时间倒序排列（最新的在前）
    EXPECT_EQ(messages[0].getId(), msg1_id.value());
    EXPECT_EQ(messages[1].getId(), msg2_id.value());
    EXPECT_EQ(messages[2].getId(), msg3_id.value());
    
    // 验证消息内容
    EXPECT_EQ(messages[0].getContent(), "First message");
    EXPECT_EQ(messages[1].getContent(), "Second message");
    EXPECT_EQ(messages[2].getContent(), "Third message");
}

TEST_F(MessageRepositoryTest, GetMessage_ValidId_Success) {
    const std::string content = "Test message for retrieval";
    auto message_id = message_repo->saveMessage(test_room1_id, test_user1_id, content);
    ASSERT_TRUE(message_id.has_value());
    
    auto message = message_repo->getMessage(message_id.value());
    
    ASSERT_TRUE(message.has_value());
    EXPECT_EQ(message->getId(), message_id.value());
    EXPECT_EQ(message->getRoomId(), test_room1_id);
    EXPECT_EQ(message->getSenderId(), test_user1_id);
    EXPECT_EQ(message->getContent(), content);
    EXPECT_EQ(message->getUserName(), "testuser1");
}

TEST_F(MessageRepositoryTest, GetMessage_InvalidId_Failure) {
    const int64_t invalid_id = 99999;
    
    auto message = message_repo->getMessage(invalid_id);
    EXPECT_FALSE(message.has_value());
}

TEST_F(MessageRepositoryTest, GetRoomMessageCount_ValidRoom_Success) {
    // 初始计数应为0
    EXPECT_EQ(message_repo->getRoomMessageCount(test_room1_id), 0);
    
    // 添加几条消息
    message_repo->saveMessage(test_room1_id, test_user1_id, "Message 1");
    message_repo->saveMessage(test_room1_id, test_user2_id, "Message 2");
    message_repo->saveMessage(test_room1_id, test_user1_id, "Message 3");
    
    // 验证计数
    EXPECT_EQ(message_repo->getRoomMessageCount(test_room1_id), 3);
    
    // 另一个房间应该仍为0
    EXPECT_EQ(message_repo->getRoomMessageCount(test_room2_id), 0);
}

// ================== 私聊消息测试 ==================

TEST_F(MessageRepositoryTest, SaveDirectMessage_ValidData_Success) {
    const std::string content = "Hello, this is a direct message!";
    
    auto message_id = message_repo->saveDirectMessage(test_user1_id, test_user2_id, content);
    
    ASSERT_TRUE(message_id.has_value());
    EXPECT_GT(message_id.value(), 0);
    
    // 验证消息存在
    EXPECT_TRUE(message_repo->directMessageExists(message_id.value()));
}

TEST_F(MessageRepositoryTest, SaveDirectMessage_InvalidUser_Failure) {
    const int64_t invalid_user_id = 99999;
    const std::string content = "This should fail";
    
    auto message_id = message_repo->saveDirectMessage(test_user1_id, invalid_user_id, content);
    
    // 由于外键约束，这应该失败
    EXPECT_FALSE(message_id.has_value());
}

TEST_F(MessageRepositoryTest, DeleteDirectMessage_ExistingMessage_Success) {
    // 首先创建一条直接消息
    auto message_id = message_repo->saveDirectMessage(test_user1_id, test_user2_id, "DM to delete");
    ASSERT_TRUE(message_id.has_value());
    
    // 删除消息
    bool deleted = message_repo->deleteDirectMessage(message_id.value());
    EXPECT_TRUE(deleted);
    
    // 验证消息不再存在
    EXPECT_FALSE(message_repo->directMessageExists(message_id.value()));
}

TEST_F(MessageRepositoryTest, GetDirectMessages_ValidUsers_Success) {
    // 创建几条测试消息
    auto msg1_id = message_repo->saveDirectMessage(test_user1_id, test_user2_id, "Hello from user1");
    auto msg2_id = message_repo->saveDirectMessage(test_user2_id, test_user1_id, "Reply from user2");
    auto msg3_id = message_repo->saveDirectMessage(test_user1_id, test_user2_id, "Another message");
    
    ASSERT_TRUE(msg1_id.has_value());
    ASSERT_TRUE(msg2_id.has_value());
    ASSERT_TRUE(msg3_id.has_value());
    
    // 获取对话消息
    auto messages = message_repo->getDirectMessages(test_user1_id, test_user2_id, 10, 0);
    
    EXPECT_EQ(messages.size(), 3);
    
    // 验证消息按时间正序排列（数据库默认排序）
    EXPECT_EQ(messages[0].getId(), msg1_id.value());
    EXPECT_EQ(messages[1].getId(), msg3_id.value());
    EXPECT_EQ(messages[2].getId(), msg2_id.value());
}

TEST_F(MessageRepositoryTest, GetDirectMessage_ValidId_Success) {
    const std::string content = "Test direct message for retrieval";
    auto message_id = message_repo->saveDirectMessage(test_user1_id, test_user2_id, content);
    ASSERT_TRUE(message_id.has_value());
    
    auto message = message_repo->getDirectMessage(message_id.value());
    
    ASSERT_TRUE(message.has_value());
    EXPECT_EQ(message->getId(), message_id.value());
    EXPECT_EQ(message->getSenderId(), test_user1_id);
    EXPECT_EQ(message->getReceiverId(), test_user2_id);
    EXPECT_EQ(message->getContent(), content);
}

TEST_F(MessageRepositoryTest, GetDirectMessageCount_ValidUsers_Success) {
    // 初始计数应为0
    EXPECT_EQ(message_repo->getDirectMessageCount(test_user1_id, test_user2_id), 0);
    
    // 添加几条消息
    message_repo->saveDirectMessage(test_user1_id, test_user2_id, "Message 1");
    message_repo->saveDirectMessage(test_user2_id, test_user1_id, "Message 2");
    message_repo->saveDirectMessage(test_user1_id, test_user2_id, "Message 3");
    
    // 验证计数
    EXPECT_EQ(message_repo->getDirectMessageCount(test_user1_id, test_user2_id), 3);
    
    // 反向查询应该返回相同结果
    EXPECT_EQ(message_repo->getDirectMessageCount(test_user2_id, test_user1_id), 3);
    
    // 其他用户对应该为0
    EXPECT_EQ(message_repo->getDirectMessageCount(test_user1_id, test_user3_id), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
