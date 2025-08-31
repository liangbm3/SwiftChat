#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "../../model/message.hpp"
#include "../../model/direct_message.hpp"
#include "../connection_pool.hpp"
#include "../mysql_statement.hpp"

namespace db {

// 消息数据访问类
class MessageRepository {
 public:
  explicit MessageRepository(ConnectionPool &pool);  // 构造函数，接受连接池引用

  // 房间消息操作
  std::optional<int64_t> saveMessage(int64_t room_id, int64_t sender_id, 
                                     const std::string &content);  // 保存房间消息，返回消息ID
  bool deleteMessage(int64_t message_id);  // 根据ID删除房间消息
  bool messageExists(int64_t message_id) const;  // 检查房间消息是否存在

  // 房间消息查询
  std::vector<Message> getRoomMessages(int64_t room_id, int limit = 50, 
                                      int offset = 0) const;  // 分页获取房间消息，倒序
  std::vector<Message> getRoomMessagesAfter(int64_t room_id, 
                                           const std::string &created_at) const;  // 获取指定时间后的房间消息，正序
  std::optional<Message> getMessage(int64_t message_id) const;  // 根据ID获取房间消息
  int64_t getRoomMessageCount(int64_t room_id) const;  // 获取房间消息总数

  // 私聊消息操作
  std::optional<int64_t> saveDirectMessage(int64_t sender_id, int64_t receiver_id,
                                           const std::string &content);  // 保存私聊消息，返回消息ID
  bool deleteDirectMessage(int64_t message_id);  // 根据ID删除私聊消息
  bool directMessageExists(int64_t message_id) const;  // 检查私聊消息是否存在

  // 私聊消息查询
  std::vector<DirectMessage> getDirectMessages(int64_t user1_id, int64_t user2_id,
                                               int limit = 50, int offset = 0) const;  // 分页获取两用户间的私聊消息，倒序
  std::vector<DirectMessage> getDirectMessagesAfter(int64_t user1_id, int64_t user2_id,
                                                    const std::string &created_at) const;  // 获取指定时间后的私聊消息，正序
  std::optional<DirectMessage> getDirectMessage(int64_t message_id) const;  // 根据ID获取私聊消息
  int64_t getDirectMessageCount(int64_t user1_id, int64_t user2_id) const;  // 获取两用户间私聊消息总数
  std::vector<int64_t> getConversationPartners(int64_t user_id) const;  // 获取用户的所有会话对象

 private:
  ConnectionPool &pool_;
};

}  // namespace db
