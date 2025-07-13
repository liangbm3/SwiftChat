#pragma once
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct User
{
    std::string username;
    std::string password;
    bool is_online;

    json toJson() const;
    static User fromJson(const json &j); // 传入JSON数据并返回一个JSON对象，是一个工厂方法
};
