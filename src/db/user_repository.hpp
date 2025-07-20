#pragma once

#include <string>
#include <vector>
#include <optional>
#include "database_connection.hpp"
#include "../model/user.hpp"

// 用户数据访问类
class UserRepository
{
public:
    explicit UserRepository(DatabaseConnection* db_conn);// 构造函数，接受数据库连接指针

    // 用户基本操作
    bool createUser(const std::string &username, const std::string &password_hash);// 创建用户
    bool validateUser(const std::string &username, const std::string &password_hash);// 验证用户
    bool userExists(const std::string &user_id);// 根据ID检查用户是否存在

    // 用户状态管理
    bool setUserOnlineStatus(const std::string &user_id, bool is_online);// 根据ID设置用户在线状态
    
    // 用户查询
    std::vector<User> getAllUsers() const;// 获取所有用户
    std::optional<User> getUserById(const std::string &user_id) const; 
    std::optional<User> getUserByUsername(const std::string &username) const;

    // 工具方法
    std::string generateUserId();// 生成用户ID

private:
    DatabaseConnection* db_conn_;
};
