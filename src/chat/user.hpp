#pragma once
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class User
{
private:
    std::string id_;//用户id
    std::string username_;//用户姓名
    std::string password_;//用户密码
    bool is_online_;//用户在线状态
    int64_t last_active_time_;//用户最后活跃时间

public:
    // 构造函数
    User() : is_online_(false), last_active_time_(0) {}//默认构造函数
    User(const std::string &id, const std::string &username, const std::string &password,
         bool is_online = false, int64_t last_active_time = 0)
        : id_(id), username_(username), password_(password),
          is_online_(is_online), last_active_time_(last_active_time) {}

    // Getter方法
    const std::string &getId() const { return id_; }
    const std::string &getUsername() const { return username_; }
    const std::string &getPassword() const { return password_; }
    bool isOnline() const { return is_online_; }
    int64_t getLastActiveTime() const { return last_active_time_; }

    // Setter方法
    void setId(const std::string &id) { id_ = id; }
    void setUsername(const std::string &username) { username_ = username; }
    void setPassword(const std::string &password) { password_ = password; }
    void setOnline(bool is_online) { is_online_ = is_online; }
    void setLastActiveTime(int64_t last_active_time) { last_active_time_ = last_active_time; }

    // JSON转换
    json toJson() const;
    static User fromJson(const json &j);
};
