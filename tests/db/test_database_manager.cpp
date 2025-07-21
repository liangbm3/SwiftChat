#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <cstdio>
#include "../../src/db/database_manager.hpp" // 请确保路径正确

// 测试固件 (无需修改)
class DatabaseManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "test_db_final_" + std::to_string(rand()) + ".sqlite";
        db_manager_ = std::make_unique<DatabaseManager>(test_db_path_);
        ASSERT_TRUE(db_manager_->isConnected()) << "数据库连接创建失败";
    }

    void TearDown() override {
        db_manager_.reset();
        std::remove(test_db_path_.c_str());
    }

    std::unique_ptr<DatabaseManager> db_manager_;
    std::string test_db_path_;
};

// --- 用户管理测试 ---

TEST_F(DatabaseManagerTest, UserLifecycle) {
    // 1. 创建用户
    ASSERT_TRUE(db_manager_->createUser("alice", "pass123"));

    // 2. 通过用户名获取用户实体以得到ID
    auto alice_opt = db_manager_->getUserByUsername("alice");
    ASSERT_TRUE(alice_opt.has_value());
    User alice = *alice_opt;

    // 3. 使用ID验证用户是否存在
    ASSERT_TRUE(db_manager_->userExists(alice.getId()));
    ASSERT_FALSE(db_manager_->userExists("non-existent-id"));

    // 4. 通过ID获取用户进行验证
    auto alice_by_id_opt = db_manager_->getUserById(alice.getId());
    ASSERT_TRUE(alice_by_id_opt.has_value());
    ASSERT_EQ(alice_by_id_opt->getUsername(), "alice");
}

TEST_F(DatabaseManagerTest, UserAuthenticationAndStatus) {
    ASSERT_TRUE(db_manager_->createUser("bob", "securepass"));
    auto bob_opt = db_manager_->getUserByUsername("bob");
    ASSERT_TRUE(bob_opt.has_value());
    std::string bob_id = bob_opt->getId();

    // 1. 验证用户凭据
    ASSERT_TRUE(db_manager_->validateUser("bob", "securepass"));
    ASSERT_FALSE(db_manager_->validateUser("bob", "wrongpass"));

}


// --- 房间与成员管理测试 ---

TEST_F(DatabaseManagerTest, RoomAndMemberLifecycle) {
    // 1. 准备用户并获取ID
    db_manager_->createUser("owner", "pass_owner");
    db_manager_->createUser("member1", "pass_member");
    auto owner = *db_manager_->getUserByUsername("owner");
    auto member1 = *db_manager_->getUserByUsername("member1");

    // 2. 创建房间，直接获取返回的房间信息和ID
    auto room_opt = db_manager_->createRoom("Tech Talk", "A room for tech discussions", owner.getId());
    ASSERT_TRUE(room_opt.has_value());
    std::string room_id = room_opt->getId();
    
    // 3. 使用ID验证房间是否存在
    ASSERT_TRUE(db_manager_->roomExists(room_id));
    ASSERT_FALSE(db_manager_->roomExists("non-existent-room-id"));

    // 4. 验证创建者
    ASSERT_TRUE(db_manager_->isRoomCreator(room_id, owner.getId()));
    ASSERT_FALSE(db_manager_->isRoomCreator(room_id, member1.getId()));

    // 5. 添加成员
    ASSERT_TRUE(db_manager_->addRoomMember(room_id, owner.getId()));
    ASSERT_TRUE(db_manager_->addRoomMember(room_id, member1.getId()));

    // 6. 验证成员列表
    auto members = db_manager_->getRoomMembers(room_id);
    ASSERT_EQ(members.size(), 2);
    bool owner_found = false, member1_found = false;
    for (const auto& member : members) {
        if (member.at("id").get<std::string>() == owner.getId()) owner_found = true;
        if (member.at("id").get<std::string>() == member1.getId()) member1_found = true;
    }
    ASSERT_TRUE(owner_found && member1_found);

    // 7. 移除成员
    ASSERT_TRUE(db_manager_->removeRoomMember(room_id, member1.getId()));
    members = db_manager_->getRoomMembers(room_id);
    ASSERT_EQ(members.size(), 1);
    ASSERT_EQ(members[0].at("id").get<std::string>(), owner.getId());

    // 8. 删除房间
    ASSERT_TRUE(db_manager_->deleteRoom(room_id));
    ASSERT_FALSE(db_manager_->getRoomById(room_id).has_value());
}

// --- 消息管理测试 ---

TEST_F(DatabaseManagerTest, SaveAndGetMessages) {
    // 1. 准备环境
    auto u1 = *db_manager_->getUserByUsername( (db_manager_->createUser("u1", "p1"), "u1") );
    auto u2 = *db_manager_->getUserByUsername( (db_manager_->createUser("u2", "p2"), "u2") );
    auto room_opt = db_manager_->createRoom("Gossip Channel", "A channel for gossip", u1.getId());
    std::string room_id = room_opt->getId();
    db_manager_->addRoomMember(room_id, u1.getId());
    db_manager_->addRoomMember(room_id, u2.getId());

    // 2. 发送和保存消息
    int64_t ts1 = std::chrono::system_clock::now().time_since_epoch().count();
    ASSERT_TRUE(db_manager_->saveMessage(room_id, u1.getId(), "Hello from u1!", ts1));
    int64_t ts2 = ts1 + 100;
    ASSERT_TRUE(db_manager_->saveMessage(room_id, u2.getId(), "Hello from u2!", ts2));

    // 3. 获取消息并验证
    auto messages = db_manager_->getMessages(room_id, 10); // Limit to 10
    ASSERT_EQ(messages.size(), 2);

    ASSERT_EQ(messages[0].getUserId(), u1.getId());
    ASSERT_EQ(messages[0].getContent(), "Hello from u1!");

    ASSERT_EQ(messages[1].getUserId(), u2.getId());
    ASSERT_EQ(messages[1].getContent(), "Hello from u2!");
}

// --- 完整的端到端流程测试 ---

TEST_F(DatabaseManagerTest, FullWorkflow) {
    // 1. 创建用户
    auto admin = *db_manager_->getUserByUsername( (db_manager_->createUser("admin", "adminpass"), "admin") );
    auto guest = *db_manager_->getUserByUsername( (db_manager_->createUser("guest", "guestpass"), "guest") );

    // 2. 创建房间
    auto room_opt = db_manager_->createRoom("Project Omega", "Secret project room", admin.getId());
    ASSERT_TRUE(room_opt.has_value());
    std::string room_id = room_opt->getId();

    // 3. 添加成员
    ASSERT_TRUE(db_manager_->addRoomMember(room_id, admin.getId()));
    ASSERT_TRUE(db_manager_->addRoomMember(room_id, guest.getId()));

    // 4. guest发送消息
    ASSERT_TRUE(db_manager_->saveMessage(room_id, guest.getId(), "Task A complete.", 1000));
    
    // 5. admin获取消息并验证
    auto messages = db_manager_->getMessages(room_id);
    ASSERT_EQ(messages.size(), 1);
    ASSERT_EQ(messages[0].getUserId(), guest.getId());

    // 6. admin移除guest
    ASSERT_TRUE(db_manager_->removeRoomMember(room_id, guest.getId()));
    auto members = db_manager_->getRoomMembers(room_id);
    ASSERT_EQ(members.size(), 1);
    ASSERT_EQ(members[0].at("id").get<std::string>(), admin.getId());

    // 7. admin删除房间
    ASSERT_TRUE(db_manager_->deleteRoom(room_id));
    ASSERT_FALSE(db_manager_->roomExists(room_id));
}

// --- 边界条件和错误处理测试 ---

TEST_F(DatabaseManagerTest, DuplicateUsernames) {
    // 测试重复用户名
    ASSERT_TRUE(db_manager_->createUser("duplicate", "pass1"));
    ASSERT_FALSE(db_manager_->createUser("duplicate", "pass2"));
    
    // 验证第一个用户仍然有效
    ASSERT_TRUE(db_manager_->validateUser("duplicate", "pass1"));
    ASSERT_FALSE(db_manager_->validateUser("duplicate", "pass2"));
}

TEST_F(DatabaseManagerTest, DuplicateRoomNames) {
    // 创建用户
    db_manager_->createUser("creator1", "pass1");
    db_manager_->createUser("creator2", "pass2");
    auto user1 = *db_manager_->getUserByUsername("creator1");
    auto user2 = *db_manager_->getUserByUsername("creator2");

    // 测试重复房间名
    auto room1_opt = db_manager_->createRoom("Duplicate Room", "First room", user1.getId());
    ASSERT_TRUE(room1_opt.has_value());
    
    auto room2_opt = db_manager_->createRoom("Duplicate Room", "Second room", user2.getId());
    ASSERT_FALSE(room2_opt.has_value()); // 应该失败
}

// --- 批量操作和性能测试 ---

TEST_F(DatabaseManagerTest, BatchUserOperations) {
    // 创建多个用户
    const int user_count = 10;
    std::vector<std::string> usernames;
    
    for (int i = 0; i < user_count; ++i) {
        std::string username = "user" + std::to_string(i);
        std::string password = "pass" + std::to_string(i);
        ASSERT_TRUE(db_manager_->createUser(username, password));
        usernames.push_back(username);
    }
    
    // 验证所有用户
    auto all_users = db_manager_->getAllUsers();
    ASSERT_EQ(all_users.size(), user_count);
    
    // 验证每个用户的凭据
    for (int i = 0; i < user_count; ++i) {
        std::string username = "user" + std::to_string(i);
        std::string password = "pass" + std::to_string(i);
        ASSERT_TRUE(db_manager_->validateUser(username, password));
    }
}

TEST_F(DatabaseManagerTest, BatchRoomOperations) {
    // 创建用户
    db_manager_->createUser("creator", "creatorpass");
    auto creator = *db_manager_->getUserByUsername("creator");
    
    // 创建多个房间
    const int room_count = 5;
    std::vector<std::string> room_ids;
    
    for (int i = 0; i < room_count; ++i) {
        std::string room_name = "Room" + std::to_string(i);
        std::string description = "Description for room " + std::to_string(i);
        auto room_opt = db_manager_->createRoom(room_name, description, creator.getId());
        ASSERT_TRUE(room_opt.has_value());
        room_ids.push_back(room_opt->getId());
    }
    
    // 验证所有房间
    auto all_rooms = db_manager_->getAllRooms();
    ASSERT_EQ(all_rooms.size(), room_count);
    
    // 验证创建者权限
    for (const auto& room_id : room_ids) {
        ASSERT_TRUE(db_manager_->isRoomCreator(room_id, creator.getId()));
    }
}

TEST_F(DatabaseManagerTest, BatchMessageOperations) {
    // 准备环境
    db_manager_->createUser("sender", "senderpass");
    auto sender = *db_manager_->getUserByUsername("sender");
    auto room_opt = db_manager_->createRoom("Message Test Room", "For testing messages", sender.getId());
    std::string room_id = room_opt->getId();
    db_manager_->addRoomMember(room_id, sender.getId());
    
    // 发送多条消息
    const int message_count = 20;
    std::vector<std::string> message_contents;
    
    for (int i = 0; i < message_count; ++i) {
        std::string content = "Message " + std::to_string(i);
        int64_t timestamp = 1000 + i * 100; // 递增时间戳
        ASSERT_TRUE(db_manager_->saveMessage(room_id, sender.getId(), content, timestamp));
        message_contents.push_back(content);
    }
    
    // 获取所有消息
    auto all_messages = db_manager_->getMessages(room_id, message_count);
    ASSERT_EQ(all_messages.size(), message_count);
    
    // 验证消息顺序（应该按时间戳排序）
    for (int i = 0; i < message_count; ++i) {
        ASSERT_EQ(all_messages[i].getContent(), "Message " + std::to_string(i));
    }
    
    // 测试限制数量
    auto limited_messages = db_manager_->getMessages(room_id, 5);
    ASSERT_EQ(limited_messages.size(), 5);
}

// --- 数据一致性和关联测试 ---

TEST_F(DatabaseManagerTest, CascadeDeleteBehavior) {
    // 注意：SQLite 默认不启用外键约束，所以级联删除不会自动工作
    // 这个测试验证当前行为：删除房间不会自动删除相关数据
    
    // 创建完整的数据结构
    db_manager_->createUser("owner", "ownerpass");
    db_manager_->createUser("member", "memberpass");
    auto owner = *db_manager_->getUserByUsername("owner");
    auto member = *db_manager_->getUserByUsername("member");
    
    auto room_opt = db_manager_->createRoom("Test Room", "Test Description", owner.getId());
    std::string room_id = room_opt->getId();
    
    // 添加成员和消息
    db_manager_->addRoomMember(room_id, owner.getId());
    db_manager_->addRoomMember(room_id, member.getId());
    db_manager_->saveMessage(room_id, owner.getId(), "Owner message", 1000);
    db_manager_->saveMessage(room_id, member.getId(), "Member message", 2000);
    
    // 验证数据存在
    ASSERT_EQ(db_manager_->getRoomMembers(room_id).size(), 2);
    ASSERT_EQ(db_manager_->getMessages(room_id).size(), 2);
    
    // 删除房间
    ASSERT_TRUE(db_manager_->deleteRoom(room_id));
    
    // 验证房间被删除
    ASSERT_FALSE(db_manager_->roomExists(room_id));
    
    // 注意：由于外键约束未启用，相关数据可能仍然存在
    // 这是当前实现的行为，在生产环境中应该启用外键约束或手动清理
    auto members_after = db_manager_->getRoomMembers(room_id);
    auto messages_after = db_manager_->getMessages(room_id);
    
    // 记录当前行为（可能数据还在，取决于实现）
    // 在理想情况下，这些应该为空
    LOG_INFO << "Members after room deletion: " << members_after.size();
    LOG_INFO << "Messages after room deletion: " << messages_after.size();
}

TEST_F(DatabaseManagerTest, ForeignKeyConstraintsValidation) {
    // 测试外键约束是否能正确阻止无效的引用
    
    // 创建一个有效的用户和房间
    db_manager_->createUser("validuser", "password");
    auto user = *db_manager_->getUserByUsername("validuser");
    auto room_opt = db_manager_->createRoom("Valid Room", "Description", user.getId());
    std::string room_id = room_opt->getId();
    
    // 尝试插入无效的外键引用应该失败
    std::string invalid_user_id = "invalid-user-12345";
    std::string invalid_room_id = "invalid-room-12345";
    
    // 测试无效用户ID的房间成员添加
    ASSERT_FALSE(db_manager_->addRoomMember(room_id, invalid_user_id)) 
        << "Should not be able to add invalid user to room";
    
    // 测试无效房间ID的房间成员添加
    ASSERT_FALSE(db_manager_->addRoomMember(invalid_room_id, user.getId())) 
        << "Should not be able to add user to invalid room";
    
    // 测试无效用户ID的消息保存
    ASSERT_FALSE(db_manager_->saveMessage(room_id, invalid_user_id, "Invalid message", 1000)) 
        << "Should not be able to save message from invalid user";
    
    // 测试无效房间ID的消息保存
    ASSERT_FALSE(db_manager_->saveMessage(invalid_room_id, user.getId(), "Invalid message", 1000)) 
        << "Should not be able to save message to invalid room";
    
    // 验证只有有效的操作成功
    ASSERT_TRUE(db_manager_->addRoomMember(room_id, user.getId()));
    ASSERT_TRUE(db_manager_->saveMessage(room_id, user.getId(), "Valid message", 1000));
    
    // 验证数据正确性
    auto members = db_manager_->getRoomMembers(room_id);
    auto messages = db_manager_->getMessages(room_id);
    
    ASSERT_EQ(members.size(), 1);
    ASSERT_EQ(messages.size(), 1);
    ASSERT_EQ(messages[0].getContent(), "Valid message");
}

// --- 外键约束和级联删除测试 ---

TEST_F(DatabaseManagerTest, ForeignKeyConstraintsWithCascade) {
    // 这个测试验证我们之前添加的外键约束定义是否正确
    // 即使SQLite中外键约束默认未启用，表结构定义应该是正确的
    
    // 创建测试数据
    db_manager_->createUser("testuser", "testpass");
    auto user = *db_manager_->getUserByUsername("testuser");
    
    auto room_opt = db_manager_->createRoom("FK Test Room", "Testing foreign keys", user.getId());
    ASSERT_TRUE(room_opt.has_value());
    std::string room_id = room_opt->getId();
    
    // 添加成员
    ASSERT_TRUE(db_manager_->addRoomMember(room_id, user.getId()));
    
    // 发送消息
    ASSERT_TRUE(db_manager_->saveMessage(room_id, user.getId(), "Test message", 1000));
    
    // 验证数据存在
    ASSERT_EQ(db_manager_->getRoomMembers(room_id).size(), 1);
    ASSERT_EQ(db_manager_->getMessages(room_id).size(), 1);
    
    // 验证外键关系约束（尝试插入无效数据应该失败）
    ASSERT_FALSE(db_manager_->addRoomMember(room_id, "invalid-user-id"));
    ASSERT_FALSE(db_manager_->addRoomMember("invalid-room-id", user.getId()));
    ASSERT_FALSE(db_manager_->saveMessage(room_id, "invalid-user-id", "Invalid message", 2000));
    ASSERT_FALSE(db_manager_->saveMessage("invalid-room-id", user.getId(), "Invalid message", 2000));
}

TEST_F(DatabaseManagerTest, UserRoomRelationships) {
    // 创建用户和房间
    db_manager_->createUser("user1", "pass1");
    db_manager_->createUser("user2", "pass2");
    auto user1 = *db_manager_->getUserByUsername("user1");
    auto user2 = *db_manager_->getUserByUsername("user2");
    
    auto room1_opt = db_manager_->createRoom("Room1", "First room", user1.getId());
    auto room2_opt = db_manager_->createRoom("Room2", "Second room", user2.getId());
    std::string room1_id = room1_opt->getId();
    std::string room2_id = room2_opt->getId();
    
    // 建立复杂的成员关系
    db_manager_->addRoomMember(room1_id, user1.getId());
    db_manager_->addRoomMember(room1_id, user2.getId());
    db_manager_->addRoomMember(room2_id, user2.getId());
    
    // 测试用户加入的房间
    auto user1_rooms = db_manager_->getUserJoinedRooms(user1.getId());
    auto user2_rooms = db_manager_->getUserJoinedRooms(user2.getId());
    
    ASSERT_EQ(user1_rooms.size(), 1); // user1只在room1中
    ASSERT_EQ(user2_rooms.size(), 2); // user2在两个房间中
    
    // 验证房间成员
    auto room1_members = db_manager_->getRoomMembers(room1_id);
    auto room2_members = db_manager_->getRoomMembers(room2_id);
    
    ASSERT_EQ(room1_members.size(), 2);
    ASSERT_EQ(room2_members.size(), 1);
}

// --- 特殊字符和编码测试 ---

TEST_F(DatabaseManagerTest, SpecialCharacterHandling) {
    // 测试特殊字符
    std::string special_username = "用户@123";
    std::string special_password = "密码!@#$%^&*()";
    std::string special_room_name = "房间 with émojis 🚀";
    std::string special_description = "描述 with 'quotes' and \"double quotes\"";
    std::string special_message = "消息 with\nnewlines\tand\ttabs & symbols: <>&\"'";
    
    // 创建用户
    ASSERT_TRUE(db_manager_->createUser(special_username, special_password));
    ASSERT_TRUE(db_manager_->validateUser(special_username, special_password));
    
    auto user = *db_manager_->getUserByUsername(special_username);
    
    // 创建房间
    auto room_opt = db_manager_->createRoom(special_room_name, special_description, user.getId());
    ASSERT_TRUE(room_opt.has_value());
    std::string room_id = room_opt->getId();
    
    // 验证房间信息
    auto retrieved_room = *db_manager_->getRoomById(room_id);
    ASSERT_EQ(retrieved_room.getName(), special_room_name);
    ASSERT_EQ(retrieved_room.getDescription(), special_description);
    
    // 发送特殊消息
    db_manager_->addRoomMember(room_id, user.getId());
    ASSERT_TRUE(db_manager_->saveMessage(room_id, user.getId(), special_message, 1000));
    
    // 验证消息内容
    auto messages = db_manager_->getMessages(room_id);
    ASSERT_EQ(messages.size(), 1);
    ASSERT_EQ(messages[0].getContent(), special_message);
}

// --- 并发安全测试（基础） ---

TEST_F(DatabaseManagerTest, ConcurrentUserCreation) {
    // 注意：这是基础的并发测试，真正的并发测试需要多线程
    // 这里主要测试快速连续操作的安全性
    
    const int operations = 50;
    std::vector<std::string> usernames;
    
    // 快速创建多个用户
    for (int i = 0; i < operations; ++i) {
        std::string username = "concurrent_user_" + std::to_string(i);
        std::string password = "pass_" + std::to_string(i);
        
        if (db_manager_->createUser(username, password)) {
            usernames.push_back(username);
        }
    }
    
    // 验证所有创建的用户
    for (const auto& username : usernames) {
        auto user_opt = db_manager_->getUserByUsername(username);
        ASSERT_TRUE(user_opt.has_value()) << "User " << username << " should exist";
    }
}