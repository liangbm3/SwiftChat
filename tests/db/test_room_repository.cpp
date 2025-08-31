#include <gtest/gtest.h>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>

#include "../../src/db/respository/room_repository.hpp"
#include "../../src/db/respository/user_repository.hpp"
#include "../../src/db/connection_pool.hpp"
#include "../../src/db/database_initializer.hpp"
#include "../../src/utils/logger.hpp"

using namespace db;

class RoomRepositoryTest : public ::testing::Test {
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
        
        // 创建 Repository 实例
        room_repo = std::make_unique<RoomRepository>(*pool);
        user_repo = std::make_unique<UserRepository>(*pool);
        
        // 清理测试数据
        cleanupTestData();
        
        // 创建测试用户
        createTestUsers();
    }

    void TearDown() override {
        // 清理测试数据
        cleanupTestData();
    }

    void cleanupTestData() {
        auto conn = ConnectionPool::getInstance().getConnection();
        if (conn) {
            // 清理房间成员关系
            mysql_query(conn->getRawConnection(), "DELETE FROM room_members WHERE room_id IN (SELECT id FROM rooms WHERE name LIKE 'test_%')");
            // 清理房间
            mysql_query(conn->getRawConnection(), "DELETE FROM rooms WHERE name LIKE 'test_%'");
            // 清理测试用户
            mysql_query(conn->getRawConnection(), "DELETE FROM users WHERE username LIKE 'test_%'");
        }
    }

    void createTestUsers() {
        // 创建两个测试用户
        auto user1_id = user_repo->createUser("test_user_1", "password_hash_1");
        auto user2_id = user_repo->createUser("test_user_2", "password_hash_2");
        
        ASSERT_TRUE(user1_id.has_value()) << "Failed to create test user 1";
        ASSERT_TRUE(user2_id.has_value()) << "Failed to create test user 2";
        
        test_user_1_id = user1_id.value();
        test_user_2_id = user2_id.value();
    }

    ConnectionPool* pool;
    std::unique_ptr<RoomRepository> room_repo;
    std::unique_ptr<UserRepository> user_repo;
    int64_t test_user_1_id;
    int64_t test_user_2_id;
};

// 测试房间创建功能
TEST_F(RoomRepositoryTest, CreateRoom) {
    std::string room_name = "test_room_create";
    
    // 测试成功创建房间
    auto room_id = room_repo->createRoom(room_name, test_user_1_id);
    ASSERT_TRUE(room_id.has_value()) << "Failed to create room";
    EXPECT_GT(room_id.value(), 0) << "Room ID should be positive";
    
    // 验证房间是否真的被创建了
    EXPECT_TRUE(room_repo->roomExists(room_id.value())) << "Room should exist after creation";
    
    // 测试重复房间名创建（应该失败，因为房间名有唯一约束）
    auto duplicate_id = room_repo->createRoom(room_name, test_user_2_id);
    EXPECT_FALSE(duplicate_id.has_value()) << "Should not allow duplicate room names";
    
    // 测试创建不同名称的房间（应该成功）
    std::string different_room_name = "test_room_create_different";
    auto different_id = room_repo->createRoom(different_room_name, test_user_2_id);
    EXPECT_TRUE(different_id.has_value()) << "Should allow different room names";
    if (different_id.has_value()) {
        EXPECT_NE(different_id.value(), room_id.value()) << "Different rooms should have different IDs";
    }
}

// 测试房间存在性检查
TEST_F(RoomRepositoryTest, RoomExists) {
    std::string room_name = "test_room_exists";
    
    // 创建测试房间
    auto room_id = room_repo->createRoom(room_name, test_user_1_id);
    ASSERT_TRUE(room_id.has_value()) << "Failed to create test room";
    
    // 测试房间存在
    EXPECT_TRUE(room_repo->roomExists(room_id.value())) << "Room should exist";
    
    // 测试不存在的房间
    EXPECT_FALSE(room_repo->roomExists(99999)) << "Non-existent room should not exist";
}

// 测试房间删除功能
TEST_F(RoomRepositoryTest, DeleteRoom) {
    std::string room_name = "test_room_delete";
    
    // 创建测试房间
    auto room_id = room_repo->createRoom(room_name, test_user_1_id);
    ASSERT_TRUE(room_id.has_value()) << "Failed to create test room";
    
    // 验证房间存在
    EXPECT_TRUE(room_repo->roomExists(room_id.value())) << "Room should exist before deletion";
    
    // 删除房间
    EXPECT_TRUE(room_repo->deleteRoom(room_id.value())) << "Should successfully delete room";
    
    // 验证房间被删除
    EXPECT_FALSE(room_repo->roomExists(room_id.value())) << "Room should not exist after deletion";
    
    // 测试删除不存在的房间
    EXPECT_FALSE(room_repo->deleteRoom(99999)) << "Should fail to delete non-existent room";
}

// 测试房间更新功能
TEST_F(RoomRepositoryTest, UpdateRoom) {
    std::string room_name = "test_room_update";
    std::string updated_name = "test_room_updated";
    std::string description = "Test room description";
    
    // 创建测试房间
    auto room_id = room_repo->createRoom(room_name, test_user_1_id);
    ASSERT_TRUE(room_id.has_value()) << "Failed to create test room";
    
    // 更新房间信息
    EXPECT_TRUE(room_repo->updateRoom(room_id.value(), updated_name, description)) 
        << "Should successfully update room";
    
    // 验证更新结果
    auto room = room_repo->getRoom(room_id.value());
    ASSERT_TRUE(room.has_value()) << "Should retrieve updated room";
    EXPECT_EQ(room->getName(), updated_name) << "Room name should be updated";
    EXPECT_EQ(room->getDescription(), description) << "Room description should be updated";
    
    // 测试更新不存在的房间
    EXPECT_FALSE(room_repo->updateRoom(99999, "new_name", "new_desc")) 
        << "Should fail to update non-existent room";
}

// 测试获取房间详细信息
TEST_F(RoomRepositoryTest, GetRoom) {
    std::string room_name = "test_room_get";
    
    // 创建测试房间
    auto room_id = room_repo->createRoom(room_name, test_user_1_id);
    ASSERT_TRUE(room_id.has_value()) << "Failed to create test room";
    
    // 测试通过ID获取房间
    auto room_by_id = room_repo->getRoom(room_id.value());
    ASSERT_TRUE(room_by_id.has_value()) << "Should retrieve room by ID";
    EXPECT_EQ(room_by_id->getId(), room_id.value()) << "Room ID should match";
    EXPECT_EQ(room_by_id->getName(), room_name) << "Room name should match";
    EXPECT_EQ(room_by_id->getCreatorId(), test_user_1_id) << "Creator ID should match";
    
    // 测试通过房间名获取房间
    auto room_by_name = room_repo->getRoom(room_name);
    ASSERT_TRUE(room_by_name.has_value()) << "Should retrieve room by name";
    EXPECT_EQ(room_by_name->getId(), room_id.value()) << "Room ID should match";
    EXPECT_EQ(room_by_name->getName(), room_name) << "Room name should match";
    
    // 测试获取不存在的房间
    auto non_existent_by_id = room_repo->getRoom(99999);
    EXPECT_FALSE(non_existent_by_id.has_value()) << "Non-existent room by ID should return nullopt";
    
    auto non_existent_by_name = room_repo->getRoom("non_existent_room");
    EXPECT_FALSE(non_existent_by_name.has_value()) << "Non-existent room by name should return nullopt";
}

// 测试根据房间名获取房间ID
TEST_F(RoomRepositoryTest, GetRoomIdByName) {
    std::string room_name = "test_room_get_id";
    
    // 创建测试房间
    auto room_id = room_repo->createRoom(room_name, test_user_1_id);
    ASSERT_TRUE(room_id.has_value()) << "Failed to create test room";
    
    // 测试根据房间名获取ID
    auto retrieved_id = room_repo->getRoomIdByName(room_name);
    ASSERT_TRUE(retrieved_id.has_value()) << "Should retrieve room ID by name";
    EXPECT_EQ(retrieved_id.value(), room_id.value()) << "Retrieved ID should match created ID";
    
    // 测试获取不存在房间的ID
    auto non_existent_id = room_repo->getRoomIdByName("non_existent_room");
    EXPECT_FALSE(non_existent_id.has_value()) << "Non-existent room should return nullopt";
}

// 测试验证房间创建者
TEST_F(RoomRepositoryTest, IsRoomCreator) {
    std::string room_name = "test_room_creator";
    
    // 创建测试房间
    auto room_id = room_repo->createRoom(room_name, test_user_1_id);
    ASSERT_TRUE(room_id.has_value()) << "Failed to create test room";
    
    // 测试正确的创建者
    EXPECT_TRUE(room_repo->isRoomCreator(test_user_1_id, room_id.value())) 
        << "Creator should be verified correctly";
    
    // 测试错误的创建者
    EXPECT_FALSE(room_repo->isRoomCreator(test_user_2_id, room_id.value())) 
        << "Non-creator should not be verified as creator";
    
    // 测试不存在的房间
    EXPECT_FALSE(room_repo->isRoomCreator(test_user_1_id, 99999)) 
        << "Non-existent room should return false";
    
    // 测试不存在的用户
    EXPECT_FALSE(room_repo->isRoomCreator(99999, room_id.value())) 
        << "Non-existent user should return false";
}

// 测试获取所有房间名称
TEST_F(RoomRepositoryTest, GetAllRoomNames) {
    // 创建多个测试房间
    std::vector<std::string> room_names = {"test_room_all_1", "test_room_all_2", "test_room_all_3"};
    std::vector<int64_t> room_ids;
    
    for (const auto& name : room_names) {
        auto room_id = room_repo->createRoom(name, test_user_1_id);
        ASSERT_TRUE(room_id.has_value()) << "Failed to create room: " << name;
        room_ids.push_back(room_id.value());
    }
    
    // 获取所有房间名称
    auto all_names = room_repo->getAllRoomNames();
    
    // 验证所有创建的房间名称都在结果中
    for (const auto& name : room_names) {
        EXPECT_TRUE(std::find(all_names.begin(), all_names.end(), name) != all_names.end()) 
            << "Room name '" << name << "' should be in the list";
    }
}

// 测试获取所有房间详细信息
TEST_F(RoomRepositoryTest, GetAllRooms) {
    // 创建多个测试房间
    std::vector<std::string> room_names = {"test_room_details_1", "test_room_details_2"};
    std::vector<int64_t> room_ids;
    
    for (const auto& name : room_names) {
        auto room_id = room_repo->createRoom(name, test_user_1_id);
        ASSERT_TRUE(room_id.has_value()) << "Failed to create room: " << name;
        room_ids.push_back(room_id.value());
    }
    
    // 获取所有房间详细信息
    auto all_rooms = room_repo->getAllRooms();
    
    // 验证所有创建的房间都在结果中
    int found_count = 0;
    for (const auto& room : all_rooms) {
        if (std::find(room_names.begin(), room_names.end(), room.getName()) != room_names.end()) {
            found_count++;
            EXPECT_EQ(room.getCreatorId(), test_user_1_id) << "Creator ID should match";
        }
    }
    EXPECT_EQ(found_count, room_names.size()) << "All created rooms should be found";
}

// 测试房间成员管理 - 添加成员
TEST_F(RoomRepositoryTest, AddRoomMember) {
    std::string room_name = "test_room_add_member";
    
    // 创建测试房间
    auto room_id = room_repo->createRoom(room_name, test_user_1_id);
    ASSERT_TRUE(room_id.has_value()) << "Failed to create test room";
    
    // 添加房间成员
    EXPECT_TRUE(room_repo->addRoomMember(room_id.value(), test_user_2_id)) 
        << "Should successfully add room member";
    
    // 验证成员被添加
    auto members = room_repo->getRoomMembers(room_id.value());
    bool found_user2 = false;
    for (const auto& member : members) {
        if (member.getId() == test_user_2_id) {
            found_user2 = true;
            break;
        }
    }
    EXPECT_TRUE(found_user2) << "User 2 should be in room members";
    
    // 测试重复添加同一成员
    EXPECT_TRUE(room_repo->addRoomMember(room_id.value(), test_user_2_id)) 
        << "Should handle duplicate member addition gracefully";
    
    // 测试添加成员到不存在的房间
    EXPECT_FALSE(room_repo->addRoomMember(99999, test_user_2_id)) 
        << "Should fail to add member to non-existent room";
    
    // 测试添加不存在的用户到房间
    EXPECT_FALSE(room_repo->addRoomMember(room_id.value(), 99999)) 
        << "Should fail to add non-existent user to room";
}

// 测试房间成员管理 - 移除成员
TEST_F(RoomRepositoryTest, RemoveRoomMember) {
    std::string room_name = "test_room_remove_member";
    
    // 创建测试房间
    auto room_id = room_repo->createRoom(room_name, test_user_1_id);
    ASSERT_TRUE(room_id.has_value()) << "Failed to create test room";
    
    // 添加房间成员
    EXPECT_TRUE(room_repo->addRoomMember(room_id.value(), test_user_2_id)) 
        << "Should successfully add room member";
    
    // 验证成员被添加
    auto members_before = room_repo->getRoomMembers(room_id.value());
    bool found_user2_before = false;
    for (const auto& member : members_before) {
        if (member.getId() == test_user_2_id) {
            found_user2_before = true;
            break;
        }
    }
    EXPECT_TRUE(found_user2_before) << "User 2 should be in room members before removal";
    
    // 移除房间成员
    EXPECT_TRUE(room_repo->removeRoomMember(room_id.value(), test_user_2_id)) 
        << "Should successfully remove room member";
    
    // 验证成员被移除
    auto members_after = room_repo->getRoomMembers(room_id.value());
    bool found_user2_after = false;
    for (const auto& member : members_after) {
        if (member.getId() == test_user_2_id) {
            found_user2_after = true;
            break;
        }
    }
    EXPECT_FALSE(found_user2_after) << "User 2 should not be in room members after removal";
    
    // 测试移除不存在的成员
    EXPECT_FALSE(room_repo->removeRoomMember(room_id.value(), test_user_2_id)) 
        << "Should fail to remove non-existent member";
    
    // 测试从不存在的房间移除成员
    EXPECT_FALSE(room_repo->removeRoomMember(99999, test_user_2_id)) 
        << "Should fail to remove member from non-existent room";
}

// 测试获取房间成员
TEST_F(RoomRepositoryTest, GetRoomMembers) {
    std::string room_name = "test_room_get_members";
    
    // 创建测试房间
    auto room_id = room_repo->createRoom(room_name, test_user_1_id);
    ASSERT_TRUE(room_id.has_value()) << "Failed to create test room";
    
    // 初始状态应该没有成员（创建者不自动成为成员）
    auto initial_members = room_repo->getRoomMembers(room_id.value());
    EXPECT_TRUE(initial_members.empty()) << "Initial room should have no members";
    
    // 添加两个成员
    EXPECT_TRUE(room_repo->addRoomMember(room_id.value(), test_user_1_id)) 
        << "Should add user 1 as member";
    EXPECT_TRUE(room_repo->addRoomMember(room_id.value(), test_user_2_id)) 
        << "Should add user 2 as member";
    
    // 获取房间成员
    auto members = room_repo->getRoomMembers(room_id.value());
    EXPECT_EQ(members.size(), 2) << "Room should have 2 members";
    
    // 验证成员信息
    std::set<int64_t> member_ids;
    for (const auto& member : members) {
        member_ids.insert(member.getId());
        EXPECT_FALSE(member.getUsername().empty()) << "Member username should not be empty";
    }
    
    EXPECT_TRUE(member_ids.count(test_user_1_id) > 0) << "User 1 should be in members";
    EXPECT_TRUE(member_ids.count(test_user_2_id) > 0) << "User 2 should be in members";
    
    // 测试获取不存在房间的成员
    auto non_existent_members = room_repo->getRoomMembers(99999);
    EXPECT_TRUE(non_existent_members.empty()) << "Non-existent room should have no members";
}

// 测试并发创建房间
TEST_F(RoomRepositoryTest, ConcurrentCreateRoom) {
    const int num_threads = 5;
    const int rooms_per_thread = 3;
    std::vector<std::thread> threads;
    std::atomic<int> success_count(0);
    std::atomic<int> failure_count(0);
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i, rooms_per_thread, &success_count, &failure_count]() {
            for (int j = 0; j < rooms_per_thread; ++j) {
                std::string room_name = "test_concurrent_room_" + std::to_string(i) + "_" + std::to_string(j);
                auto room_id = room_repo->createRoom(room_name, test_user_1_id);
                if (room_id.has_value()) {
                    success_count++;
                } else {
                    failure_count++;
                }
                
                // 小延迟以增加并发冲突的可能性
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 验证结果
    int total_attempts = num_threads * rooms_per_thread;
    EXPECT_EQ(success_count.load() + failure_count.load(), total_attempts) 
        << "All attempts should be accounted for";
    EXPECT_GT(success_count.load(), 0) << "At least some rooms should be created successfully";
    
    // 大部分应该成功（允许少量失败由于并发竞争）
    EXPECT_GE(success_count.load(), total_attempts * 0.8) 
        << "Most room creations should succeed";
}

// 测试数据库连接失败场景（需要模拟连接池耗尽）
TEST_F(RoomRepositoryTest, DatabaseConnectionFailure) {
    // 这个测试需要在连接池资源耗尽的情况下进行
    // 由于我们的连接池大小为5，我们创建多个并发操作来耗尽连接
    
    const int num_threads = 10;  // 超过连接池大小
    std::vector<std::thread> threads;
    std::atomic<int> success_count(0);
    std::atomic<int> failure_count(0);
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i, &success_count, &failure_count]() {
            // 长时间占用连接的操作
            for (int j = 0; j < 2; ++j) {
                std::string room_name = "test_conn_fail_room_" + std::to_string(i) + "_" + std::to_string(j);
                auto room_id = room_repo->createRoom(room_name, test_user_1_id);
                if (room_id.has_value()) {
                    success_count++;
                } else {
                    failure_count++;
                }
                
                // 模拟长时间操作
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 在高并发下，应该有一些操作成功，可能有一些失败
    EXPECT_GT(success_count.load(), 0) << "Some operations should succeed";
    
    // 打印统计信息用于调试
    LOG_INFO << "Connection failure test - Success: " << success_count.load() 
             << ", Failure: " << failure_count.load();
}
