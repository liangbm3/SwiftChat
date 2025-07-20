#include <gtest/gtest.h>
#include "../../src/chat/user.hpp"

// 测试User对象的基本功能
TEST(UserTest, BasicFunctionality) {
    User user("123", "testuser", "testpass", true);
    
    EXPECT_EQ(user.getId(), "123");
    EXPECT_EQ(user.getUsername(), "testuser");
    EXPECT_EQ(user.getPassword(), "testpass");
    EXPECT_TRUE(user.isOnline());
}

// 测试User的setter方法
TEST(UserTest, SetterMethods) {
    User user;
    
    user.setId("456");
    user.setUsername("newuser");
    user.setPassword("newpass");
    user.setOnline(true);
    
    EXPECT_EQ(user.getId(), "456");
    EXPECT_EQ(user.getUsername(), "newuser");
    EXPECT_EQ(user.getPassword(), "newpass");
    EXPECT_TRUE(user.isOnline());
}

// 测试User对象转JSON
TEST(UserTest, ToJson) {
    User user("789", "jsonuser", "jsonpass", false);
    
    json j = user.toJson();
    
    EXPECT_EQ(j["id"], "789");
    EXPECT_EQ(j["username"], "jsonuser");
    EXPECT_EQ(j["password"], "jsonpass");
    EXPECT_EQ(j["is_online"], false);
}

// 测试从JSON创建User对象
TEST(UserTest, FromJson) {
    json j;
    j["id"] = "999";
    j["username"] = "fromjsonuser";
    j["password"] = "fromjsonpass";
    j["is_online"] = true;
    
    User user = User::fromJson(j);
    
    EXPECT_EQ(user.getId(), "999");
    EXPECT_EQ(user.getUsername(), "fromjsonuser");
    EXPECT_EQ(user.getPassword(), "fromjsonpass");
    EXPECT_TRUE(user.isOnline());
}

// 测试JSON往返转换
TEST(UserTest, JsonRoundTrip) {
    User originalUser("round123", "roundtripuser", "complexpass!@#", true);
    
    // 转换为JSON再转回User
    json j = originalUser.toJson();
    User reconstructedUser = User::fromJson(j);
    
    // 验证所有字段都正确
    EXPECT_EQ(originalUser.getId(), reconstructedUser.getId());
    EXPECT_EQ(originalUser.getUsername(), reconstructedUser.getUsername());
    EXPECT_EQ(originalUser.getPassword(), reconstructedUser.getPassword());
    EXPECT_EQ(originalUser.isOnline(), reconstructedUser.isOnline());
    EXPECT_EQ(originalUser.getPassword(), reconstructedUser.getPassword());
    EXPECT_EQ(originalUser.isOnline(), reconstructedUser.isOnline());
}

// 测试边界情况
TEST(UserTest, EdgeCases) {
    // 测试空字符串
    User emptyUser("", "", "", false);
    json j = emptyUser.toJson();
    User reconstructed = User::fromJson(j);
    
    EXPECT_EQ(reconstructed.getId(), "");
    EXPECT_EQ(reconstructed.getUsername(), "");
    EXPECT_EQ(reconstructed.getPassword(), "");
    EXPECT_FALSE(reconstructed.isOnline());
    
    // 测试长字符串
    std::string longString(1000, 'a');
    User longUser("longid", longString, longString, true);
    json longJson = longUser.toJson();
    User longReconstructed = User::fromJson(longJson);
    
    EXPECT_EQ(longReconstructed.getUsername(), longString);
    EXPECT_EQ(longReconstructed.getPassword(), longString);
}

// 测试特殊字符
TEST(UserTest, SpecialCharacters) {
    User specialUser("special", "用户名测试", "密码测试🔐", true);
    
    json j = specialUser.toJson();
    User reconstructed = User::fromJson(j);
    
    EXPECT_EQ(reconstructed.getUsername(), "用户名测试");
    EXPECT_EQ(reconstructed.getPassword(), "密码测试🔐");
    EXPECT_TRUE(reconstructed.isOnline());
}
