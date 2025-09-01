#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>

#include <nlohmann/json.hpp>
#include "model/message.hpp"
#include "model/direct_message.hpp"
#include "model/room.hpp"
#include "model/user.hpp"
#include "connection_pool.hpp"
#include "respository/message_repository.hpp"
#include "respository/room_repository.hpp"
#include "respository/user_repository.hpp"


class DatabaseManager {
 public:
  explicit DatabaseManager(const db::MySQLConfig &config, size_t pool_size = 10);
  ~DatabaseManager() = default;

  // 检查数据库连接状态
  bool isConnected() const;

  // 用户操作代理
  std::optional<int64_t> createUser(const std::string &username,
                                    const std::string &password_hash);
  bool deleteUser(int64_t user_id);
  bool validateUser(const std::string &username,
                    const std::string &password_hash);
  bool userExists(int64_t user_id);
  bool userExists(const std::string &username);
  bool updateUserStatus(int64_t user_id, int status);
  bool updateLastSeen(int64_t user_id);
  
  std::vector<User> getAllUsers() const;
  std::optional<User> getUser(int64_t user_id) const;
  std::optional<User> getUser(const std::string &username) const;
  std::vector<User> getOnlineUsers() const;

  // 房间操作代理
  std::optional<int64_t> createRoom(const std::string &name, int64_t creator_id);
  bool deleteRoom(int64_t room_id);
  bool roomExists(int64_t room_id) const;
  bool updateRoom(int64_t room_id, const std::string &name,
                  const std::string &description);
  
  std::vector<std::string> getAllRoomNames() const;
  std::vector<Room> getAllRooms() const;
  std::optional<Room> getRoom(int64_t room_id) const;
  std::optional<Room> getRoom(const std::string &room_name) const;
  std::optional<int64_t> getRoomIdByName(const std::string &room_name) const;
  bool isRoomCreator(int64_t user_id, int64_t room_id) const;

  // 房间成员操作代理
  std::vector<User> getRoomMembers(int64_t room_id) const;
  bool addRoomMember(int64_t room_id, int64_t user_id);
  bool removeRoomMember(int64_t room_id, int64_t user_id);

  // 房间消息操作代理
  std::optional<int64_t> saveMessage(int64_t room_id, int64_t sender_id,
                                     const std::string &content);
  bool deleteMessage(int64_t message_id);
  bool messageExists(int64_t message_id) const;
  
  std::vector<Message> getRoomMessages(int64_t room_id, int limit = 50,
                                      int offset = 0) const;
  std::vector<Message> getRoomMessagesAfter(int64_t room_id,
                                           const std::string &created_at) const;
  std::optional<Message> getMessage(int64_t message_id) const;
  int64_t getRoomMessageCount(int64_t room_id) const;

  // 私聊消息操作代理
  std::optional<int64_t> saveDirectMessage(int64_t sender_id, int64_t receiver_id,
                                           const std::string &content);
  bool deleteDirectMessage(int64_t message_id);
  bool directMessageExists(int64_t message_id) const;
  
  std::vector<DirectMessage> getDirectMessages(int64_t user1_id, int64_t user2_id,
                                               int limit = 50, int offset = 0) const;
  std::vector<DirectMessage> getDirectMessagesAfter(int64_t user1_id, int64_t user2_id,
                                                    const std::string &created_at) const;
  std::optional<DirectMessage> getDirectMessage(int64_t message_id) const;
  int64_t getDirectMessageCount(int64_t user1_id, int64_t user2_id) const;
  std::vector<int64_t> getConversationPartners(int64_t user_id) const;

  // 获取各个仓库的直接访问（如果需要更复杂的操作）
  db::UserRepository *getUserRepository() { return user_repo_.get(); }
  db::RoomRepository *getRoomRepository() { return room_repo_.get(); }
  db::MessageRepository *getMessageRepository() { return message_repo_.get(); }

 private:
  std::unique_ptr<db::UserRepository> user_repo_;         // 用户仓库
  std::unique_ptr<db::RoomRepository> room_repo_;         // 房间仓库
  std::unique_ptr<db::MessageRepository> message_repo_;   // 消息仓库
};
