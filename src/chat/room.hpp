#pragma once
#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class Room
{
private:
    std::string id_;           // 房间ID
    std::string name_;         // 房间名称
    std::string description_;  // 房间描述
    std::string creator_id_;   // 创建者ID
    int64_t created_at_;       // 创建时间戳

public:
    // 构造函数
    Room() : created_at_(0) {} // 默认构造函数
    Room(const std::string &id, const std::string &name, const std::string &description,
         const std::string &creator_id, int64_t created_at)
        : id_(id), name_(name), description_(description), creator_id_(creator_id), created_at_(created_at) {}

    // Getter方法
    const std::string &getId() const { return id_; }
    const std::string &getName() const { return name_; }
    const std::string &getDescription() const { return description_; }
    const std::string &getCreatorId() const { return creator_id_; }
    int64_t getCreatedAt() const { return created_at_; }

    // Setter方法
    void setId(const std::string &id) { id_ = id; }
    void setName(const std::string &name) { name_ = name; }
    void setDescription(const std::string &description) { description_ = description; }
    void setCreatorId(const std::string &creator_id) { creator_id_ = creator_id; }
    void setCreatedAt(int64_t created_at) { created_at_ = created_at; }

    // JSON转换
    json toJson() const;
    static Room fromJson(const json &j);
};
