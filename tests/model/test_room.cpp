#include <gtest/gtest.h>

#include "model/room.hpp"

// 测试Room对象的基本功能
TEST(RoomTest, BasicFunctionality) {
  Room room(123, "Test Room", "A test room for testing", 456,
            "2021-12-31 23:59:59");

  EXPECT_EQ(room.getId(), 123);
  EXPECT_EQ(room.getName(), "Test Room");
  EXPECT_EQ(room.getDescription(), "A test room for testing");
  EXPECT_EQ(room.getCreatorId(), 456);
  EXPECT_EQ(room.getCreatedAt(), "2021-12-31 23:59:59");
}

// 测试Room的setter方法
TEST(RoomTest, SetterMethods) {
  Room room;

  room.setId(456);
  room.setName("Updated Room");
  room.setDescription("Updated description");
  room.setCreatorId(789);
  room.setCreatedAt("2022-01-01 00:00:00");

  EXPECT_EQ(room.getId(), 456);
  EXPECT_EQ(room.getName(), "Updated Room");
  EXPECT_EQ(room.getDescription(), "Updated description");
  EXPECT_EQ(room.getCreatorId(), 789);
  EXPECT_EQ(room.getCreatedAt(), "2022-01-01 00:00:00");
}

// 测试Room对象转JSON
TEST(RoomTest, ToJson) {
  Room room(200, "JSON Room", "A room for JSON testing",
            300, "2022-01-01 12:00:00");

  json j = room.toJson();

  EXPECT_EQ(j["id"], 200);
  EXPECT_EQ(j["name"], "JSON Room");
  EXPECT_EQ(j["description"], "A room for JSON testing");
  EXPECT_EQ(j["creator_id"], 300);
  EXPECT_EQ(j["created_at"], "2022-01-01 12:00:00");
}

// 测试从JSON创建Room对象
TEST(RoomTest, FromJson) {
  json j;
  j["id"] = 400;
  j["name"] = "Room from JSON";
  j["description"] = "Created from JSON object";
  j["creator_id"] = 500;
  j["created_at"] = "2022-01-01 13:00:00";

  Room room = Room::fromJson(j);

  EXPECT_EQ(room.getId(), 400);
  EXPECT_EQ(room.getName(), "Room from JSON");
  EXPECT_EQ(room.getDescription(), "Created from JSON object");
  EXPECT_EQ(room.getCreatorId(), 500);
  EXPECT_EQ(room.getCreatedAt(), "2022-01-01 13:00:00");
}

// 测试默认构造函数
TEST(RoomTest, DefaultConstructor) {
  Room room;

  EXPECT_EQ(room.getId(), 0);
  EXPECT_EQ(room.getName(), "");
  EXPECT_EQ(room.getDescription(), "");
  EXPECT_EQ(room.getCreatorId(), 0);
  EXPECT_EQ(room.getCreatedAt(), "");
}

// 测试JSON转换的完整循环
TEST(RoomTest, JsonRoundTrip) {
  Room original_room(600, "Roundtrip Room",
                     "Testing roundtrip conversion", 700,
                     "2022-01-01 14:00:00");

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
  json j;  // 空的JSON对象

  Room room = Room::fromJson(j);

  // 应该返回默认值
  EXPECT_EQ(room.getId(), 0);
  EXPECT_EQ(room.getName(), "");
  EXPECT_EQ(room.getDescription(), "");
  EXPECT_EQ(room.getCreatorId(), 0);
  EXPECT_EQ(room.getCreatedAt(), "");
}

// 测试处理部分JSON字段的情况
TEST(RoomTest, FromPartialJson) {
  json j;
  j["id"] = 800;
  j["name"] = "Partial Room";
  // 故意省略description, creator_id和created_at

  Room room = Room::fromJson(j);

  EXPECT_EQ(room.getId(), 800);
  EXPECT_EQ(room.getName(), "Partial Room");
  EXPECT_EQ(room.getDescription(), "");  // 应该是默认值
  EXPECT_EQ(room.getCreatorId(), 0);    // 应该是默认值
  EXPECT_EQ(room.getCreatedAt(), "");     // 应该是默认值
}

// 测试带有特殊字符的房间名称和描述
TEST(RoomTest, SpecialCharacters) {
  Room room(900, "房间 🏠", "这是一个测试房间 with émojis! 😀",
            1000, "2022-01-01 15:00:00");

  json j = room.toJson();
  Room restored_room = Room::fromJson(j);

  EXPECT_EQ(restored_room.getName(), "房间 🏠");
  EXPECT_EQ(restored_room.getDescription(), "这是一个测试房间 with émojis! 😀");
  EXPECT_EQ(restored_room.getCreatorId(), 1000);
}

// 测试空字符串字段
TEST(RoomTest, EmptyFields) {
  Room room(1100, "", "", 1200, "2022-01-01 16:00:00");

  json j = room.toJson();
  Room restored_room = Room::fromJson(j);

  EXPECT_EQ(restored_room.getId(), 1100);
  EXPECT_EQ(restored_room.getName(), "");
  EXPECT_EQ(restored_room.getDescription(), "");
  EXPECT_EQ(restored_room.getCreatorId(), 1200);
  EXPECT_EQ(restored_room.getCreatedAt(), "2022-01-01 16:00:00");
}
