#include <gtest/gtest.h>

#include "model/user.hpp"

// 测试User对象的基本功能
TEST(UserTest, BasicFunctionality) {
  User user(123, "testuser", "testpass");

  EXPECT_EQ(user.getId(), 123);
  EXPECT_EQ(user.getUsername(), "testuser");
  EXPECT_EQ(user.getPassword(), "testpass");
  EXPECT_EQ(user.getStatus(), 0);  // 默认离线状态
  EXPECT_EQ(user.getLastSeen(), "");
}

// 测试User的setter方法
TEST(UserTest, SetterMethods) {
  User user;

  user.setId(456);
  user.setUsername("newuser");
  user.setPassword("newpass");
  user.setStatus(1);  // 在线状态
  user.setLastSeen("2022-01-01 12:00:00");

  EXPECT_EQ(user.getId(), 456);
  EXPECT_EQ(user.getUsername(), "newuser");
  EXPECT_EQ(user.getPassword(), "newpass");
  EXPECT_EQ(user.getStatus(), 1);
  EXPECT_EQ(user.getLastSeen(), "2022-01-01 12:00:00");
}

// 测试User对象转JSON
TEST(UserTest, ToJson) {
  User user(789, "jsonuser", "jsonpass", 1, "2022-01-01 13:00:00");

  json j = user.toJson();

  EXPECT_EQ(j["id"], 789);
  EXPECT_EQ(j["username"], "jsonuser");
  EXPECT_EQ(j["password"], "jsonpass");
  EXPECT_EQ(j["status"], 1);
  EXPECT_EQ(j["last_seen"], "2022-01-01 13:00:00");
}

// 测试从JSON创建User对象
TEST(UserTest, FromJson) {
  json j;
  j["id"] = 999;
  j["username"] = "fromjsonuser";
  j["password"] = "fromjsonpass";
  j["status"] = 1;
  j["last_seen"] = "2022-01-01 14:00:00";

  User user = User::fromJson(j);

  EXPECT_EQ(user.getId(), 999);
  EXPECT_EQ(user.getUsername(), "fromjsonuser");
  EXPECT_EQ(user.getPassword(), "fromjsonpass");
  EXPECT_EQ(user.getStatus(), 1);
  EXPECT_EQ(user.getLastSeen(), "2022-01-01 14:00:00");
}

// 测试JSON往返转换
TEST(UserTest, JsonRoundTrip) {
  User originalUser(1234, "roundtripuser", "complexpass!@#", 1, "2022-01-01 15:00:00");

  // 转换为JSON再转回User
  json j = originalUser.toJson();
  User reconstructedUser = User::fromJson(j);

  // 验证所有字段都正确
  EXPECT_EQ(originalUser.getId(), reconstructedUser.getId());
  EXPECT_EQ(originalUser.getUsername(), reconstructedUser.getUsername());
  EXPECT_EQ(originalUser.getPassword(), reconstructedUser.getPassword());
  EXPECT_EQ(originalUser.getStatus(), reconstructedUser.getStatus());
  EXPECT_EQ(originalUser.getLastSeen(), reconstructedUser.getLastSeen());
}

// 测试边界情况
TEST(UserTest, EdgeCases) {
  // 测试空字符串
  User emptyUser(0, "", "");
  json j = emptyUser.toJson();
  User reconstructed = User::fromJson(j);

  EXPECT_EQ(reconstructed.getId(), 0);
  EXPECT_EQ(reconstructed.getUsername(), "");
  EXPECT_EQ(reconstructed.getPassword(), "");
  EXPECT_EQ(reconstructed.getStatus(), 0);
  EXPECT_EQ(reconstructed.getLastSeen(), "");

  // 测试长字符串
  std::string longString(1000, 'a');
  User longUser(5678, longString, longString);
  json longJson = longUser.toJson();
  User longReconstructed = User::fromJson(longJson);

  EXPECT_EQ(longReconstructed.getUsername(), longString);
  EXPECT_EQ(longReconstructed.getPassword(), longString);
}

// 测试特殊字符
TEST(UserTest, SpecialCharacters) {
  User specialUser(9999, "用户名测试", "密码测试🔐", 1, "2022-01-01 16:00:00");

  json j = specialUser.toJson();
  User reconstructed = User::fromJson(j);

  EXPECT_EQ(reconstructed.getUsername(), "用户名测试");
  EXPECT_EQ(reconstructed.getPassword(), "密码测试🔐");
  EXPECT_EQ(reconstructed.getStatus(), 1);
  EXPECT_EQ(reconstructed.getLastSeen(), "2022-01-01 16:00:00");
}

// 测试默认构造函数
TEST(UserTest, DefaultConstructor) {
  User user;

  EXPECT_EQ(user.getId(), 0);
  EXPECT_EQ(user.getUsername(), "");
  EXPECT_EQ(user.getPassword(), "");
  EXPECT_EQ(user.getStatus(), 0);
  EXPECT_EQ(user.getLastSeen(), "");
}

// 测试状态字段
TEST(UserTest, StatusField) {
  User user(1111, "statususer", "statuspass");
  
  // 测试默认状态
  EXPECT_EQ(user.getStatus(), 0);

  // 测试设置在线状态
  user.setStatus(1);
  EXPECT_EQ(user.getStatus(), 1);

  // 测试JSON转换
  json j = user.toJson();
  User reconstructed = User::fromJson(j);
  EXPECT_EQ(reconstructed.getStatus(), 1);
}

// 测试最后在线时间字段
TEST(UserTest, LastSeenField) {
  User user(2222, "timeuser", "timepass");
  
  // 测试默认值
  EXPECT_EQ(user.getLastSeen(), "");

  // 测试设置时间
  user.setLastSeen("2022-01-01 17:00:00");
  EXPECT_EQ(user.getLastSeen(), "2022-01-01 17:00:00");

  // 测试JSON转换
  json j = user.toJson();
  User reconstructed = User::fromJson(j);
  EXPECT_EQ(reconstructed.getLastSeen(), "2022-01-01 17:00:00");
}
