#include <gtest/gtest.h>

#include "model/message.hpp"

// 测试Message对象的基本功能
TEST(MessageTest, BasicFunctionality) {
  Message message(1, 123, 456, "Hello, World!", "2021-12-31 23:59:59",
                  "testuser");

  EXPECT_EQ(message.getId(), 1);
  EXPECT_EQ(message.getRoomId(), 123);
  EXPECT_EQ(message.getSenderId(), 456);
  EXPECT_EQ(message.getContent(), "Hello, World!");
  EXPECT_EQ(message.getCreatedAt(), "2021-12-31 23:59:59");
  EXPECT_EQ(message.getUserName(), "testuser");
}

// 测试Message的用户名功能
TEST(MessageTest, MessageWithUserName) {
  Message message(2, 789, 456, "Hello with username!", "2021-12-31 23:59:59",
                  "alice");

  EXPECT_EQ(message.getId(), 2);
  EXPECT_EQ(message.getRoomId(), 789);
  EXPECT_EQ(message.getSenderId(), 456);
  EXPECT_EQ(message.getContent(), "Hello with username!");
  EXPECT_EQ(message.getCreatedAt(), "2021-12-31 23:59:59");
  EXPECT_EQ(message.getUserName(), "alice");
}

// 测试Message的setter方法
TEST(MessageTest, SetterMethods) {
  Message message;

  message.setId(5);
  message.setRoomId(123);
  message.setSenderId(789);
  message.setContent("Updated content");
  message.setCreatedAt("2022-01-01 00:00:00");
  message.setUserName("newsender");

  EXPECT_EQ(message.getId(), 5);
  EXPECT_EQ(message.getRoomId(), 123);
  EXPECT_EQ(message.getSenderId(), 789);
  EXPECT_EQ(message.getContent(), "Updated content");
  EXPECT_EQ(message.getCreatedAt(), "2022-01-01 00:00:00");
  EXPECT_EQ(message.getUserName(), "newsender");
}

// 测试Message对象转JSON
TEST(MessageTest, ToJsonWithUserName) {
  Message message(10, 200, 300, "JSON test message", "2022-01-01 12:00:00",
                  "jsonuser");

  json j = message.toJson();

  EXPECT_EQ(j["id"], 10);
  EXPECT_EQ(j["room_id"], 200);
  EXPECT_EQ(j["sender_id"], 300);
  EXPECT_EQ(j["content"], "JSON test message");
  EXPECT_EQ(j["created_at"], "2022-01-01 12:00:00");
  EXPECT_EQ(j["user_name"], "jsonuser");
}

// 测试Message对象转JSON（不含用户名）
TEST(MessageTest, ToJsonWithEmptyUserName) {
  Message message(11, 201, 301, "JSON test without username",
                  "2022-01-01 13:00:00", "");

  json j = message.toJson();

  EXPECT_EQ(j["id"], 11);
  EXPECT_EQ(j["room_id"], 201);
  EXPECT_EQ(j["sender_id"], 301);
  EXPECT_EQ(j["content"], "JSON test without username");
  EXPECT_EQ(j["created_at"], "2022-01-01 13:00:00");
  EXPECT_EQ(j["user_name"], "");
}

// 测试从JSON创建Message对象（含用户名）
TEST(MessageTest, FromJsonWithUserName) {
  json j;
  j["id"] = 20;
  j["room_id"] = 400;
  j["sender_id"] = 500;
  j["content"] = "Message from JSON";
  j["created_at"] = "2022-01-01 14:00:00";
  j["user_name"] = "jsonuser";

  Message message = Message::fromJson(j);

  EXPECT_EQ(message.getId(), 20);
  EXPECT_EQ(message.getRoomId(), 400);
  EXPECT_EQ(message.getSenderId(), 500);
  EXPECT_EQ(message.getContent(), "Message from JSON");
  EXPECT_EQ(message.getCreatedAt(), "2022-01-01 14:00:00");
  EXPECT_EQ(message.getUserName(), "jsonuser");
}

// 测试从JSON创建Message对象（不含用户名）
TEST(MessageTest, FromJsonWithoutUserName) {
  json j;
  j["id"] = 21;
  j["room_id"] = 401;
  j["sender_id"] = 501;
  j["content"] = "Message from JSON without username";
  j["created_at"] = "2022-01-01 15:00:00";

  Message message = Message::fromJson(j);

  EXPECT_EQ(message.getId(), 21);
  EXPECT_EQ(message.getRoomId(), 401);
  EXPECT_EQ(message.getSenderId(), 501);
  EXPECT_EQ(message.getContent(), "Message from JSON without username");
  EXPECT_EQ(message.getCreatedAt(), "2022-01-01 15:00:00");
  EXPECT_EQ(message.getUserName(), "");  // 应该是空字符串
}

// 测试默认构造函数
TEST(MessageTest, DefaultConstructor) {
  Message message;

  EXPECT_EQ(message.getId(), 0);
  EXPECT_EQ(message.getRoomId(), 0);
  EXPECT_EQ(message.getSenderId(), 0);
  EXPECT_EQ(message.getContent(), "");
  EXPECT_EQ(message.getCreatedAt(), "");
  EXPECT_EQ(message.getUserName(), "");
}

// 测试JSON转换的完整循环
TEST(MessageTest, JsonRoundTrip) {
  Message original_message(100, 600, 700,
                           "Roundtrip test", "2022-01-01 16:00:00", "roundtripuser");

  // 转换为JSON
  json j = original_message.toJson();

  // 从JSON创建新的Message对象
  Message restored_message = Message::fromJson(j);

  // 验证所有字段都正确恢复
  EXPECT_EQ(restored_message.getId(), original_message.getId());
  EXPECT_EQ(restored_message.getRoomId(), original_message.getRoomId());
  EXPECT_EQ(restored_message.getSenderId(), original_message.getSenderId());
  EXPECT_EQ(restored_message.getContent(), original_message.getContent());
  EXPECT_EQ(restored_message.getCreatedAt(), original_message.getCreatedAt());
  EXPECT_EQ(restored_message.getUserName(), original_message.getUserName());
}

// 测试处理无效JSON的情况
TEST(MessageTest, FromInvalidJson) {
  json j;  // 空的JSON对象

  Message message = Message::fromJson(j);

  // 应该返回默认值
  EXPECT_EQ(message.getId(), 0);
  EXPECT_EQ(message.getRoomId(), 0);
  EXPECT_EQ(message.getSenderId(), 0);
  EXPECT_EQ(message.getContent(), "");
  EXPECT_EQ(message.getCreatedAt(), "");
  EXPECT_EQ(message.getUserName(), "");
}
