#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <cstdint>

using json = nlohmann::json;

class User {
 private:
  int64_t id_;            // 用户ID (BIGINT AUTO_INCREMENT)
  std::string username_;  // 用户姓名
  std::string password_;  // 用户密码
  int status_;            // 用户状态 (0=离线, 1=在线)
  std::string last_seen_; // 最后在线时间戳 (TIMESTAMP格式)

 public:
  // 构造函数
  User() : id_(0), status_(0), last_seen_("") {}  // 默认构造函数
  User(int64_t id, const std::string &username, const std::string &password,
       int status = 0, const std::string &last_seen = "")
      : id_(id), username_(username), password_(password), status_(status), last_seen_(last_seen) {}

  // Getter方法
  int64_t getId() const { return id_; }
  const std::string &getUsername() const { return username_; }
  const std::string &getPassword() const { return password_; }
  int getStatus() const { return status_; }
  const std::string &getLastSeen() const { return last_seen_; }

  // Setter方法
  void setId(int64_t id) { id_ = id; }
  void setUsername(const std::string &username) { username_ = username; }
  void setPassword(const std::string &password) { password_ = password; }
  void setStatus(int status) { status_ = status; }
  void setLastSeen(const std::string &last_seen) { last_seen_ = last_seen; }

  // JSON转换
  json toJson() const;
  static User fromJson(const json &j);
};
