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

public:
    // 构造函数
    User() {}//默认构造函数
    User(const std::string &id, const std::string &username, const std::string &password)
        : id_(id), username_(username), password_(password) {}

    // Getter方法
    const std::string &getId() const { return id_; }
    const std::string &getUsername() const { return username_; }
    const std::string &getPassword() const { return password_; }

    // Setter方法
    void setId(const std::string &id) { id_ = id; }
    void setUsername(const std::string &username) { username_ = username; }
    void setPassword(const std::string &password) { password_ = password; }

    // JSON转换
    json toJson() const;
    static User fromJson(const json &j);
};
