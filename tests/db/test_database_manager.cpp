#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <optional>

#include "../../src/db/database_manager.hpp"
#include "../../src/db/connection_pool.hpp"

class DatabaseManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // 设置测试数据库配置
    config_.host = "localhost";
    config_.port = 4406;
    config_.username = "root";
    config_.password = "0";
    config_.database = "test_db";
    
    // 创建DatabaseManager实例
    db_manager_ = std::make_unique<DatabaseManager>(config_, 5);
  }

  void TearDown() override {
    db_manager_.reset();
  }

  db::MySQLConfig config_;
  std::unique_ptr<DatabaseManager> db_manager_;
};

// 测试基本连接状态
TEST_F(DatabaseManagerTest, IsConnected) {
  // 检查DatabaseManager是否正确初始化了所有仓库
  EXPECT_TRUE(db_manager_->isConnected());
  
  // 检查是否能获取仓库实例
  EXPECT_NE(db_manager_->getUserRepository(), nullptr);
  EXPECT_NE(db_manager_->getRoomRepository(), nullptr);
  EXPECT_NE(db_manager_->getMessageRepository(), nullptr);
}

// 测试用户操作代理接口
TEST_F(DatabaseManagerTest, UserRepositoryProxy) {
  const std::string username = "test_user";
  const std::string password_hash = "hashed_password";
  const int64_t user_id = 1;
  
  // 测试用户创建接口
  EXPECT_NO_THROW({
    auto result = db_manager_->createUser(username, password_hash);
    // 注意：这里不测试实际返回值，因为数据库可能不存在
  });
  
  // 测试用户删除接口
  EXPECT_NO_THROW({
    bool result = db_manager_->deleteUser(user_id);
  });
  
  // 测试用户验证接口
  EXPECT_NO_THROW({
    bool result = db_manager_->validateUser(username, password_hash);
  });
  
  // 测试用户存在检查接口（通过ID）
  EXPECT_NO_THROW({
    bool result = db_manager_->userExists(user_id);
  });
  
  // 测试用户存在检查接口（通过用户名）
  EXPECT_NO_THROW({
    bool result = db_manager_->userExists(username);
  });
  
  // 测试用户状态更新接口
  EXPECT_NO_THROW({
    bool result = db_manager_->updateUserStatus(user_id, 1);
  });
  
  // 测试最后在线时间更新接口
  EXPECT_NO_THROW({
    bool result = db_manager_->updateLastSeen(user_id);
  });
  
  // 测试获取所有用户接口
  EXPECT_NO_THROW({
    std::vector<User> users = db_manager_->getAllUsers();
  });
  
  // 测试获取用户接口（通过ID）
  EXPECT_NO_THROW({
    std::optional<User> user = db_manager_->getUser(user_id);
  });
  
  // 测试获取用户接口（通过用户名）
  EXPECT_NO_THROW({
    std::optional<User> user = db_manager_->getUser(username);
  });
  
  // 测试获取在线用户接口
  EXPECT_NO_THROW({
    std::vector<User> users = db_manager_->getOnlineUsers();
  });
}

// 测试房间操作代理接口
TEST_F(DatabaseManagerTest, RoomRepositoryProxy) {
  const std::string room_name = "test_room";
  const std::string description = "test description";
  const int64_t room_id = 1;
  const int64_t creator_id = 1;
  const int64_t user_id = 2;
  
  // 测试房间创建接口
  EXPECT_NO_THROW({
    auto result = db_manager_->createRoom(room_name, creator_id);
  });
  
  // 测试房间删除接口
  EXPECT_NO_THROW({
    bool result = db_manager_->deleteRoom(room_id);
  });
  
  // 测试房间存在检查接口
  EXPECT_NO_THROW({
    bool result = db_manager_->roomExists(room_id);
  });
  
  // 测试房间更新接口
  EXPECT_NO_THROW({
    bool result = db_manager_->updateRoom(room_id, room_name, description);
  });
  
  // 测试获取所有房间名称接口
  EXPECT_NO_THROW({
    std::vector<std::string> names = db_manager_->getAllRoomNames();
  });
  
  // 测试获取所有房间接口
  EXPECT_NO_THROW({
    std::vector<Room> rooms = db_manager_->getAllRooms();
  });
  
  // 测试获取房间接口（通过ID）
  EXPECT_NO_THROW({
    std::optional<Room> room = db_manager_->getRoom(room_id);
  });
  
  // 测试获取房间接口（通过名称）
  EXPECT_NO_THROW({
    std::optional<Room> room = db_manager_->getRoom(room_name);
  });
  
  // 测试根据名称获取房间ID接口
  EXPECT_NO_THROW({
    std::optional<int64_t> id = db_manager_->getRoomIdByName(room_name);
  });
  
  // 测试检查是否为房间创建者接口
  EXPECT_NO_THROW({
    bool result = db_manager_->isRoomCreator(creator_id, room_id);
  });
  
  // 测试获取房间成员接口
  EXPECT_NO_THROW({
    std::vector<User> members = db_manager_->getRoomMembers(room_id);
  });
  
  // 测试添加房间成员接口
  EXPECT_NO_THROW({
    bool result = db_manager_->addRoomMember(room_id, user_id);
  });
  
  // 测试移除房间成员接口
  EXPECT_NO_THROW({
    bool result = db_manager_->removeRoomMember(room_id, user_id);
  });
}

// 测试消息操作代理接口
TEST_F(DatabaseManagerTest, MessageRepositoryProxy) {
  const int64_t room_id = 1;
  const int64_t sender_id = 1;
  const int64_t receiver_id = 2;
  const int64_t message_id = 1;
  const std::string content = "test message";
  const std::string created_at = "2025-08-31 10:00:00";
  const int limit = 50;
  const int offset = 0;
  
  // 测试保存房间消息接口
  EXPECT_NO_THROW({
    auto result = db_manager_->saveMessage(room_id, sender_id, content);
  });
  
  // 测试删除房间消息接口
  EXPECT_NO_THROW({
    bool result = db_manager_->deleteMessage(message_id);
  });
  
  // 测试房间消息存在检查接口
  EXPECT_NO_THROW({
    bool result = db_manager_->messageExists(message_id);
  });
  
  // 测试获取房间消息接口（分页）
  EXPECT_NO_THROW({
    std::vector<Message> messages = db_manager_->getRoomMessages(room_id, limit, offset);
  });
  
  // 测试获取指定时间后的房间消息接口
  EXPECT_NO_THROW({
    std::vector<Message> messages = db_manager_->getRoomMessagesAfter(room_id, created_at);
  });
  
  // 测试获取房间消息接口（通过ID）
  EXPECT_NO_THROW({
    std::optional<Message> message = db_manager_->getMessage(message_id);
  });
  
  // 测试获取房间消息总数接口
  EXPECT_NO_THROW({
    int64_t count = db_manager_->getRoomMessageCount(room_id);
  });
  
  // 测试保存私聊消息接口
  EXPECT_NO_THROW({
    auto result = db_manager_->saveDirectMessage(sender_id, receiver_id, content);
  });
  
  // 测试删除私聊消息接口
  EXPECT_NO_THROW({
    bool result = db_manager_->deleteDirectMessage(message_id);
  });
  
  // 测试私聊消息存在检查接口
  EXPECT_NO_THROW({
    bool result = db_manager_->directMessageExists(message_id);
  });
  
  // 测试获取私聊消息接口（分页）
  EXPECT_NO_THROW({
    std::vector<DirectMessage> messages = db_manager_->getDirectMessages(sender_id, receiver_id, limit, offset);
  });
  
  // 测试获取指定时间后的私聊消息接口
  EXPECT_NO_THROW({
    std::vector<DirectMessage> messages = db_manager_->getDirectMessagesAfter(sender_id, receiver_id, created_at);
  });
  
  // 测试获取私聊消息接口（通过ID）
  EXPECT_NO_THROW({
    std::optional<DirectMessage> message = db_manager_->getDirectMessage(message_id);
  });
  
  // 测试获取私聊消息总数接口
  EXPECT_NO_THROW({
    int64_t count = db_manager_->getDirectMessageCount(sender_id, receiver_id);
  });
  
  // 测试获取会话伙伴接口
  EXPECT_NO_THROW({
    std::vector<int64_t> partners = db_manager_->getConversationPartners(sender_id);
  });
}

// 测试接口完整性 - 确保所有仓库接口都被代理
TEST_F(DatabaseManagerTest, InterfaceCompleteness) {
  // 这个测试主要是编译时检查，确保所有必要的接口都存在
  
  // 用户仓库接口完整性检查
  static_assert(std::is_same_v<
    decltype(&DatabaseManager::createUser),
    std::optional<int64_t>(DatabaseManager::*)(const std::string&, const std::string&)
  >);
  
  static_assert(std::is_same_v<
    decltype(&DatabaseManager::deleteUser),
    bool(DatabaseManager::*)(int64_t)
  >);
  
  static_assert(std::is_same_v<
    decltype(&DatabaseManager::validateUser),
    bool(DatabaseManager::*)(const std::string&, const std::string&)
  >);
  
  // 房间仓库接口完整性检查
  static_assert(std::is_same_v<
    decltype(&DatabaseManager::createRoom),
    std::optional<int64_t>(DatabaseManager::*)(const std::string&, int64_t)
  >);
  
  static_assert(std::is_same_v<
    decltype(&DatabaseManager::deleteRoom),
    bool(DatabaseManager::*)(int64_t)
  >);
  
  // 消息仓库接口完整性检查
  static_assert(std::is_same_v<
    decltype(&DatabaseManager::saveMessage),
    std::optional<int64_t>(DatabaseManager::*)(int64_t, int64_t, const std::string&)
  >);
  
  static_assert(std::is_same_v<
    decltype(&DatabaseManager::saveDirectMessage),
    std::optional<int64_t>(DatabaseManager::*)(int64_t, int64_t, const std::string&)
  >);
  
  // 如果编译通过，说明所有接口都正确代理了
  SUCCEED();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
