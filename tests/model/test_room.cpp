#include <gtest/gtest.h>
#include "../../src/model/room.hpp"

// 测试Room对象的基本功能
TEST(RoomTest, BasicFunctionality) {
    Room room("room_123", "Test Room", "A test room for testing", "user_creator", 1640995200);
    
    EXPECT_EQ(room.getId(), "room_123");
    EXPECT_EQ(room.getName(), "Test Room");
    EXPECT_EQ(room.getDescription(), "A test room for testing");
    EXPECT_EQ(room.getCreatorId(), "user_creator");
    EXPECT_EQ(room.getCreatedAt(), 1640995200);
}

// 测试Room的setter方法
TEST(RoomTest, SetterMethods) {
    Room room;
    
    room.setId("room_456");
    room.setName("Updated Room");
    room.setDescription("Updated description");
    room.setCreatorId("user_new_creator");
    room.setCreatedAt(1640995300);
    
    EXPECT_EQ(room.getId(), "room_456");
    EXPECT_EQ(room.getName(), "Updated Room");
    EXPECT_EQ(room.getDescription(), "Updated description");
    EXPECT_EQ(room.getCreatorId(), "user_new_creator");
    EXPECT_EQ(room.getCreatedAt(), 1640995300);
}

// 测试Room对象转JSON
TEST(RoomTest, ToJson) {
    Room room("room_json", "JSON Room", "A room for JSON testing", "user_json_creator", 1640995400);
    
    json j = room.toJson();
    
    EXPECT_EQ(j["id"], "room_json");
    EXPECT_EQ(j["name"], "JSON Room");
    EXPECT_EQ(j["description"], "A room for JSON testing");
    EXPECT_EQ(j["creator_id"], "user_json_creator");
    EXPECT_EQ(j["created_at"], 1640995400);
}

// 测试从JSON创建Room对象
TEST(RoomTest, FromJson) {
    json j;
    j["id"] = "room_from_json";
    j["name"] = "Room from JSON";
    j["description"] = "Created from JSON object";
    j["creator_id"] = "user_from_json_creator";
    j["created_at"] = 1640995500;
    
    Room room = Room::fromJson(j);
    
    EXPECT_EQ(room.getId(), "room_from_json");
    EXPECT_EQ(room.getName(), "Room from JSON");
    EXPECT_EQ(room.getDescription(), "Created from JSON object");
    EXPECT_EQ(room.getCreatorId(), "user_from_json_creator");
    EXPECT_EQ(room.getCreatedAt(), 1640995500);
}

// 测试默认构造函数
TEST(RoomTest, DefaultConstructor) {
    Room room;
    
    EXPECT_EQ(room.getId(), "");
    EXPECT_EQ(room.getName(), "");
    EXPECT_EQ(room.getDescription(), "");
    EXPECT_EQ(room.getCreatorId(), "");
    EXPECT_EQ(room.getCreatedAt(), 0);
}

// 测试JSON转换的完整循环
TEST(RoomTest, JsonRoundTrip) {
    Room original_room("room_roundtrip", "Roundtrip Room", "Testing roundtrip conversion", "user_roundtrip_creator", 1640995600);
    
    // 转换为JSON
    json j = original_room.toJson();
    
    // 从JSON创建新的Room对象
    Room restored_room = Room::fromJson(j);
    
    // 验证所有字段都正确恢复
    EXPECT_EQ(restored_room.getId(), original_room.getId());
    EXPECT_EQ(restored_room.getName(), original_room.getName());
    EXPECT_EQ(restored_room.getDescription(), original_room.getDescription());
    EXPECT_EQ(restored_room.getCreatorId(), original_room.getCreatorId());
    EXPECT_EQ(restored_room.getCreatedAt(), original_room.getCreatedAt());
}

// 测试处理无效JSON的情况
TEST(RoomTest, FromInvalidJson) {
    json j; // 空的JSON对象
    
    Room room = Room::fromJson(j);
    
    // 应该返回默认值
    EXPECT_EQ(room.getId(), "");
    EXPECT_EQ(room.getName(), "");
    EXPECT_EQ(room.getDescription(), "");
    EXPECT_EQ(room.getCreatorId(), "");
    EXPECT_EQ(room.getCreatedAt(), 0);
}

// 测试处理部分JSON字段的情况
TEST(RoomTest, FromPartialJson) {
    json j;
    j["id"] = "room_partial";
    j["name"] = "Partial Room";
    // 故意省略description, creator_id和created_at
    
    Room room = Room::fromJson(j);
    
    EXPECT_EQ(room.getId(), "room_partial");
    EXPECT_EQ(room.getName(), "Partial Room");
    EXPECT_EQ(room.getDescription(), ""); // 应该是默认值
    EXPECT_EQ(room.getCreatorId(), "");   // 应该是默认值
    EXPECT_EQ(room.getCreatedAt(), 0);    // 应该是默认值
}

// 测试带有特殊字符的房间名称和描述
TEST(RoomTest, SpecialCharacters) {
    Room room("room_special", "房间 🏠", "这是一个测试房间 with émojis! 😀", "user_创建者", 1640995700);
    
    json j = room.toJson();
    Room restored_room = Room::fromJson(j);
    
    EXPECT_EQ(restored_room.getName(), "房间 🏠");
    EXPECT_EQ(restored_room.getDescription(), "这是一个测试房间 with émojis! 😀");
    EXPECT_EQ(restored_room.getCreatorId(), "user_创建者");
}

// 测试空字符串字段
TEST(RoomTest, EmptyFields) {
    Room room("room_empty", "", "", "", 0);
    
    json j = room.toJson();
    Room restored_room = Room::fromJson(j);
    
    EXPECT_EQ(restored_room.getId(), "room_empty");
    EXPECT_EQ(restored_room.getName(), "");
    EXPECT_EQ(restored_room.getDescription(), "");
    EXPECT_EQ(restored_room.getCreatorId(), "");
    EXPECT_EQ(restored_room.getCreatedAt(), 0);
}

// 测试大的时间戳值
TEST(RoomTest, LargeTimestamp) {
    int64_t large_timestamp = 9223372036854775807LL; // int64_t的最大值
    Room room("room_large_ts", "Large Timestamp Room", "Testing large timestamp", "user_large", large_timestamp);
    
    json j = room.toJson();
    Room restored_room = Room::fromJson(j);
    
    EXPECT_EQ(restored_room.getCreatedAt(), large_timestamp);
}
