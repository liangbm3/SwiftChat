#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

class Room {
 private:
  int64_t id_;           // 房间ID (BIGINT AUTO_INCREMENT)
  std::string name_;     // 房间名称
  std::string description_;  // 房间描述
  int64_t creator_id_;   // 创建者ID (BIGINT)
  std::string created_at_;   // 创建时间戳 (TIMESTAMP格式)

 public:
  // 构造函数
  Room() : id_(0), creator_id_(0), created_at_("") {}  // 默认构造函数
  Room(int64_t id, const std::string &name,
       const std::string &description, int64_t creator_id,
       const std::string &created_at)
      : id_(id),
        name_(name),
        description_(description),
        creator_id_(creator_id),
        created_at_(created_at) {}

  // Getter方法
  int64_t getId() const { return id_; }
  const std::string &getName() const { return name_; }
  const std::string &getDescription() const { return description_; }
  int64_t getCreatorId() const { return creator_id_; }
  const std::string &getCreatedAt() const { return created_at_; }

  // Setter方法
  void setId(int64_t id) { id_ = id; }
  void setName(const std::string &name) { name_ = name; }
  void setDescription(const std::string &description) {
    description_ = description;
  }
  void setCreatorId(int64_t creator_id) { creator_id_ = creator_id; }
  void setCreatedAt(const std::string &created_at) { created_at_ = created_at; }

  // JSON转换
  json toJson() const;
  static Room fromJson(const json &j);
};
