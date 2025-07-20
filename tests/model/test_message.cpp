#include <gtest/gtest.h>
#include "../../src/model/message.hpp"
#include "../../src/model/user.hpp"

// 测试Message对象的基本功能
TEST(MessageTest, BasicFunctionality) {
    Message message(1, "room_123", "user_456", "Hello, World!", 1640995200);
    
    EXPECT_EQ(message.getId(), 1);
    EXPECT_EQ(message.getRoomId(), "room_123");
    EXPECT_EQ(message.getUserId(), "user_456");
    EXPECT_EQ(message.getContent(), "Hello, World!");
    EXPECT_EQ(message.getTimestamp(), 1640995200);
    EXPECT_FALSE(message.hasSenderInfo());
}

// 测试带发送者信息的Message对象
TEST(MessageTest, MessageWithSender) {
    User sender("user_456", "testuser", "testpass");
    Message message(2, "room_789", "user_456", "Hello with sender!", 1640995300, sender);
    
    EXPECT_EQ(message.getId(), 2);
    EXPECT_EQ(message.getRoomId(), "room_789");
    EXPECT_EQ(message.getUserId(), "user_456");
    EXPECT_EQ(message.getContent(), "Hello with sender!");
    EXPECT_EQ(message.getTimestamp(), 1640995300);
    EXPECT_TRUE(message.hasSenderInfo());
    EXPECT_EQ(message.getSender().getId(), "user_456");
    EXPECT_EQ(message.getSender().getUsername(), "testuser");
}

// 测试Message的setter方法
TEST(MessageTest, SetterMethods) {
    Message message;
    User sender("user_789", "newsender", "newpass");
    
    message.setId(5);
    message.setRoomId("room_abc");
    message.setUserId("user_xyz");
    message.setContent("Updated content");
    message.setTimestamp(1640995400);
    message.setSender(sender);
    
    EXPECT_EQ(message.getId(), 5);
    EXPECT_EQ(message.getRoomId(), "room_abc");
    EXPECT_EQ(message.getUserId(), "user_xyz");
    EXPECT_EQ(message.getContent(), "Updated content");
    EXPECT_EQ(message.getTimestamp(), 1640995400);
    EXPECT_TRUE(message.hasSenderInfo());
    EXPECT_EQ(message.getSender().getId(), "user_789");
    EXPECT_EQ(message.getSender().getUsername(), "newsender");
}

// 测试Message对象转JSON（不含发送者信息）
TEST(MessageTest, ToJsonWithoutSender) {
    Message message(10, "room_json", "user_json", "JSON test message", 1640995500);
    
    json j = message.toJson();
    
    EXPECT_EQ(j["id"], 10);
    EXPECT_EQ(j["room_id"], "room_json");
    EXPECT_EQ(j["user_id"], "user_json");
    EXPECT_EQ(j["content"], "JSON test message");
    EXPECT_EQ(j["timestamp"], 1640995500);
    EXPECT_FALSE(j.contains("sender"));
}

// 测试Message对象转JSON（含发送者信息）
TEST(MessageTest, ToJsonWithSender) {
    User sender("user_json", "jsonuser", "jsonpass");
    Message message(11, "room_json2", "user_json", "JSON test with sender", 1640995600, sender);
    
    json j = message.toJson();
    
    EXPECT_EQ(j["id"], 11);
    EXPECT_EQ(j["room_id"], "room_json2");
    EXPECT_EQ(j["user_id"], "user_json");
    EXPECT_EQ(j["content"], "JSON test with sender");
    EXPECT_EQ(j["timestamp"], 1640995600);
    EXPECT_TRUE(j.contains("sender"));
    EXPECT_EQ(j["sender"]["id"], "user_json");
    EXPECT_EQ(j["sender"]["username"], "jsonuser");
}

// 测试从JSON创建Message对象（不含发送者信息）
TEST(MessageTest, FromJsonWithoutSender) {
    json j;
    j["id"] = 20;
    j["room_id"] = "room_from_json";
    j["user_id"] = "user_from_json";
    j["content"] = "Message from JSON";
    j["timestamp"] = 1640995700;
    
    Message message = Message::fromJson(j);
    
    EXPECT_EQ(message.getId(), 20);
    EXPECT_EQ(message.getRoomId(), "room_from_json");
    EXPECT_EQ(message.getUserId(), "user_from_json");
    EXPECT_EQ(message.getContent(), "Message from JSON");
    EXPECT_EQ(message.getTimestamp(), 1640995700);
    EXPECT_FALSE(message.hasSenderInfo());
}

// 测试从JSON创建Message对象（含发送者信息）
TEST(MessageTest, FromJsonWithSender) {
    json j;
    j["id"] = 21;
    j["room_id"] = "room_from_json2";
    j["user_id"] = "user_from_json2";
    j["content"] = "Message from JSON with sender";
    j["timestamp"] = 1640995800;
    j["sender"] = {
        {"id", "user_from_json2"},
        {"username", "jsonuser2"},
        {"password", "jsonpass2"},
    };
    
    Message message = Message::fromJson(j);
    
    EXPECT_EQ(message.getId(), 21);
    EXPECT_EQ(message.getRoomId(), "room_from_json2");
    EXPECT_EQ(message.getUserId(), "user_from_json2");
    EXPECT_EQ(message.getContent(), "Message from JSON with sender");
    EXPECT_EQ(message.getTimestamp(), 1640995800);
    EXPECT_TRUE(message.hasSenderInfo());
    EXPECT_EQ(message.getSender().getId(), "user_from_json2");
    EXPECT_EQ(message.getSender().getUsername(), "jsonuser2");
}

// 测试默认构造函数
TEST(MessageTest, DefaultConstructor) {
    Message message;
    
    EXPECT_EQ(message.getId(), 0);
    EXPECT_EQ(message.getRoomId(), "");
    EXPECT_EQ(message.getUserId(), "");
    EXPECT_EQ(message.getContent(), "");
    EXPECT_EQ(message.getTimestamp(), 0);
    EXPECT_FALSE(message.hasSenderInfo());
}

// 测试JSON转换的完整循环
TEST(MessageTest, JsonRoundTrip) {
    User original_sender("user_roundtrip", "roundtripuser", "roundtrippass");
    Message original_message(100, "room_roundtrip", "user_roundtrip", "Roundtrip test", 1640995900, original_sender);
    
    // 转换为JSON
    json j = original_message.toJson();
    
    // 从JSON创建新的Message对象
    Message restored_message = Message::fromJson(j);
    
    // 验证所有字段都正确恢复
    EXPECT_EQ(restored_message.getId(), original_message.getId());
    EXPECT_EQ(restored_message.getRoomId(), original_message.getRoomId());
    EXPECT_EQ(restored_message.getUserId(), original_message.getUserId());
    EXPECT_EQ(restored_message.getContent(), original_message.getContent());
    EXPECT_EQ(restored_message.getTimestamp(), original_message.getTimestamp());
    EXPECT_TRUE(restored_message.hasSenderInfo());
    EXPECT_EQ(restored_message.getSender().getId(), original_message.getSender().getId());
    EXPECT_EQ(restored_message.getSender().getUsername(), original_message.getSender().getUsername());
}

// 测试处理无效JSON的情况
TEST(MessageTest, FromInvalidJson) {
    json j; // 空的JSON对象
    
    Message message = Message::fromJson(j);
    
    // 应该返回默认值
    EXPECT_EQ(message.getId(), 0);
    EXPECT_EQ(message.getRoomId(), "");
    EXPECT_EQ(message.getUserId(), "");
    EXPECT_EQ(message.getContent(), "");
    EXPECT_EQ(message.getTimestamp(), 0);
    EXPECT_FALSE(message.hasSenderInfo());
}
