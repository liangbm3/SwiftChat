#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "../../model/user.hpp"
#include "../connection_pool.hpp"
#include "../mysql_statement.hpp"

namespace db {

// 用户数据访问类
class UserRepository {
 public:
  explicit UserRepository(ConnectionPool &pool);  // 构造函数，接受连接池引用

  // 用户基本操作
  std::optional<int64_t> createUser(const std::string &username,
                                    const std::string &password_hash);
  bool deleteUser(int64_t user_id);         // 根据ID删除用户
  bool userExists(int64_t user_id);
  bool userExists(const std::string &username);
  bool validateUser(const std::string &username,
                    const std::string &password_hash);

  // 用户状态管理
  bool updateUserStatus(int64_t user_id,
                        int status);     // 更新用户状态 (0=离线, 1=在线)
  bool updateLastSeen(int64_t user_id);  // 更新用户最后在线时间

  // 用户查询
  std::vector<User> getAllUsers() const;  // 获取所有用户，需要分页
  std::optional<User> getUser(int64_t user_id) const;
  std::optional<User> getUser(const std::string &username) const;
  std::vector<User> getOnlineUsers() const;  // 获取在线用户列表

 private:
  ConnectionPool &pool_;
};

}  // namespace db