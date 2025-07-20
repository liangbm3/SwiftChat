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

    // 2. 更新并验证在线状态
    ASSERT_TRUE(db_manager_->setUserOnlineStatus(bob_id, true));
    bob_opt = db_manager_->getUserById(bob_id);
    ASSERT_TRUE(bob_opt->isOnline());

    ASSERT_TRUE(db_manager_->setUserOnlineStatus(bob_id, false));
    bob_opt = db_manager_->getUserById(bob_id);
    ASSERT_FALSE(bob_opt->isOnline());
}


// --- 房间与成员管理测试 ---

TEST_F(DatabaseManagerTest, RoomAndMemberLifecycle) {
    // 1. 准备用户并获取ID
    db_manager_->createUser("owner", "pass_owner");
    db_manager_->createUser("member1", "pass_member");
    auto owner = *db_manager_->getUserByUsername("owner");
    auto member1 = *db_manager_->getUserByUsername("member1");

    // 2. 创建房间，直接获取返回的房间信息和ID
    auto room_opt = db_manager_->createRoom("Tech Talk", owner.getId());
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
    auto room_opt = db_manager_->createRoom("Gossip Channel", u1.getId());
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

    ASSERT_EQ(messages[0].at("sender").at("id").get<std::string>(), u1.getId());
    ASSERT_EQ(messages[0].at("content").get<std::string>(), "Hello from u1!");

    ASSERT_EQ(messages[1].at("sender").at("id").get<std::string>(), u2.getId());
    ASSERT_EQ(messages[1].at("content").get<std::string>(), "Hello from u2!");
}

// --- 完整的端到端流程测试 ---

TEST_F(DatabaseManagerTest, FullWorkflow) {
    // 1. 创建用户
    auto admin = *db_manager_->getUserByUsername( (db_manager_->createUser("admin", "adminpass"), "admin") );
    auto guest = *db_manager_->getUserByUsername( (db_manager_->createUser("guest", "guestpass"), "guest") );

    // 2. 创建房间
    auto room_opt = db_manager_->createRoom("Project Omega", admin.getId());
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
    ASSERT_EQ(messages[0].at("sender").at("id").get<std::string>(), guest.getId());

    // 6. admin移除guest
    ASSERT_TRUE(db_manager_->removeRoomMember(room_id, guest.getId()));
    auto members = db_manager_->getRoomMembers(room_id);
    ASSERT_EQ(members.size(), 1);
    ASSERT_EQ(members[0].at("id").get<std::string>(), admin.getId());

    // 7. admin删除房间
    ASSERT_TRUE(db_manager_->deleteRoom(room_id));
    ASSERT_FALSE(db_manager_->roomExists(room_id));
}