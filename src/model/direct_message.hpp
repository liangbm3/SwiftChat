#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

class DirectMessage {
 private:
  int64_t id_;             // 消息ID (BIGINT AUTO_INCREMENT)
  int64_t sender_id_;      // 发送者用户ID (BIGINT)
  int64_t receiver_id_;    // 接收者用户ID (BIGINT)
  std::string content_;    // 消息内容
  std::string created_at_; // 时间戳 (TIMESTAMP)

 public:
  // 构造函数
  DirectMessage() : id_(0), sender_id_(0), receiver_id_(0) {}  // 默认构造函数
  DirectMessage(int64_t id, int64_t sender_id, int64_t receiver_id,
                const std::string &content, const std::string &created_at)
      : id_(id),
        sender_id_(sender_id),
        receiver_id_(receiver_id),
        content_(content),
        created_at_(created_at) {}

  // Getter方法
  int64_t getId() const { return id_; }
  int64_t getSenderId() const { return sender_id_; }
  int64_t getReceiverId() const { return receiver_id_; }
  const std::string &getContent() const { return content_; }
  const std::string &getCreatedAt() const { return created_at_; }

  // Setter方法
  void setId(int64_t id) { id_ = id; }
  void setSenderId(int64_t sender_id) { sender_id_ = sender_id; }
  void setReceiverId(int64_t receiver_id) { receiver_id_ = receiver_id; }
  void setContent(const std::string &content) { content_ = content; }
  void setCreatedAt(const std::string &created_at) { created_at_ = created_at; }

  // JSON转换
  json toJson() const;
  static DirectMessage fromJson(const json &j);
};
