#include "user_status_manager.hpp"
#include "db/database_manager.hpp"
#include "utils/logger.hpp"
#include <algorithm>

UserStatusManager::UserStatusManager(DatabaseManager& db_manager)
    : db_manager_(db_manager), running_(false)
{
    LOG_INFO << "UserStatusManager initialized";
}

UserStatusManager::~UserStatusManager()
{
    stop();
}

void UserStatusManager::start()
{
    if (running_.load()) {
        LOG_WARN << "UserStatusManager is already running";
        return;
    }
    
    running_ = true;
    heartbeat_thread_ = std::thread(&UserStatusManager::heartbeatCheckLoop, this);
    LOG_INFO << "UserStatusManager started";
}

void UserStatusManager::stop()
{
    if (!running_.load()) {
        return;
    }
    
    running_ = false;
    
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }
    
    // 清理所有在线用户状态
    {
        std::lock_guard<std::mutex> lock(status_mutex_);
        for (const auto& [user_id, session] : online_users_) {
            updateUserOnlineStatusInDB(user_id, false);
        }
        online_users_.clear();
        websocket_connections_.clear();
    }
    
    LOG_INFO << "UserStatusManager stopped";
}

void UserStatusManager::userLogin(const std::string& user_id, const std::string& connection_type, void* connection_handle)
{
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    auto session = std::make_shared<UserSession>(user_id, connection_type, connection_handle);
    online_users_[user_id] = session;
    
    if (connection_type == "websocket" && connection_handle) {
        websocket_connections_[connection_handle] = user_id;
    }
    
    updateUserOnlineStatusInDB(user_id, true);
    broadcastUserStatusChange(user_id, true);
    
    LOG_INFO << "User " << user_id << " logged in via " << connection_type;
}

void UserStatusManager::userLogout(const std::string& user_id)
{
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    auto it = online_users_.find(user_id);
    if (it != online_users_.end()) {
        // 如果是WebSocket连接，也要清理连接映射
        if (it->second->connection_type == "websocket" && it->second->connection_handle) {
            websocket_connections_.erase(it->second->connection_handle);
        }
        
        online_users_.erase(it);
        updateUserOnlineStatusInDB(user_id, false);
        broadcastUserStatusChange(user_id, false);
        
        LOG_INFO << "User " << user_id << " logged out";
    }
}

void UserStatusManager::userLogoutByConnection(void* connection_handle)
{
    if (!connection_handle) return;
    
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    auto conn_it = websocket_connections_.find(connection_handle);
    if (conn_it != websocket_connections_.end()) {
        std::string user_id = conn_it->second;
        websocket_connections_.erase(conn_it);
        
        auto user_it = online_users_.find(user_id);
        if (user_it != online_users_.end()) {
            online_users_.erase(user_it);
            updateUserOnlineStatusInDB(user_id, false);
            broadcastUserStatusChange(user_id, false);
            
            LOG_INFO << "User " << user_id << " logged out by connection close";
        }
    }
}

void UserStatusManager::updateHeartbeat(const std::string& user_id)
{
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    auto it = online_users_.find(user_id);
    if (it != online_users_.end()) {
        it->second->last_heartbeat = std::chrono::system_clock::now();
    }
}

void UserStatusManager::updateHeartbeatByConnection(void* connection_handle)
{
    if (!connection_handle) return;
    
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    auto conn_it = websocket_connections_.find(connection_handle);
    if (conn_it != websocket_connections_.end()) {
        std::string user_id = conn_it->second;
        auto user_it = online_users_.find(user_id);
        if (user_it != online_users_.end()) {
            user_it->second->last_heartbeat = std::chrono::system_clock::now();
        }
    }
}

bool UserStatusManager::isUserOnline(const std::string& user_id) const
{
    std::lock_guard<std::mutex> lock(status_mutex_);
    return online_users_.find(user_id) != online_users_.end();
}

std::vector<std::string> UserStatusManager::getOnlineUsers() const
{
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    std::vector<std::string> users;
    users.reserve(online_users_.size());
    
    for (const auto& [user_id, session] : online_users_) {
        users.push_back(user_id);
    }
    
    return users;
}

std::vector<std::string> UserStatusManager::getOnlineUsersInRoom(const std::string& room_id) const
{
    // 获取房间成员
    auto room_members = db_manager_.getRoomMembers(room_id);
    std::vector<std::string> online_members;
    
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    for (const auto& member : room_members) {
        std::string member_id;
        if (member.contains("id")) {
            member_id = member["id"].get<std::string>();
        } else if (member.contains("user_id")) {
            member_id = member["user_id"].get<std::string>();
        }
        
        if (!member_id.empty() && online_users_.find(member_id) != online_users_.end()) {
            online_members.push_back(member_id);
        }
    }
    
    return online_members;
}

size_t UserStatusManager::getOnlineUserCount() const
{
    std::lock_guard<std::mutex> lock(status_mutex_);
    return online_users_.size();
}

std::shared_ptr<UserStatusManager::UserSession> UserStatusManager::getUserSession(const std::string& user_id) const
{
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    auto it = online_users_.find(user_id);
    if (it != online_users_.end()) {
        return it->second;
    }
    
    return nullptr;
}

std::chrono::system_clock::time_point UserStatusManager::getLastActivity(const std::string& user_id) const
{
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    auto it = online_users_.find(user_id);
    if (it != online_users_.end()) {
        return it->second->last_heartbeat;
    }
    
    return std::chrono::system_clock::time_point{};
}

std::chrono::duration<double> UserStatusManager::getOnlineDuration(const std::string& user_id) const
{
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    auto it = online_users_.find(user_id);
    if (it != online_users_.end()) {
        auto now = std::chrono::system_clock::now();
        return now - it->second->login_time;
    }
    
    return std::chrono::duration<double>::zero();
}

void UserStatusManager::registerWebSocketConnection(const std::string& user_id, void* connection_handle)
{
    if (!connection_handle) return;
    
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    websocket_connections_[connection_handle] = user_id;
    
    auto it = online_users_.find(user_id);
    if (it != online_users_.end()) {
        it->second->connection_handle = connection_handle;
        it->second->connection_type = "websocket";
        it->second->last_heartbeat = std::chrono::system_clock::now();
    }
}

void UserStatusManager::unregisterWebSocketConnection(void* connection_handle)
{
    if (!connection_handle) return;
    
    userLogoutByConnection(connection_handle);
}

std::string UserStatusManager::getUserIdByConnection(void* connection_handle) const
{
    if (!connection_handle) return "";
    
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    auto it = websocket_connections_.find(connection_handle);
    if (it != websocket_connections_.end()) {
        return it->second;
    }
    
    return "";
}

UserStatusManager::OnlineStats UserStatusManager::getOnlineStats() const
{
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    OnlineStats stats;
    stats.total_online = online_users_.size();
    stats.websocket_connections = 0;
    stats.http_sessions = 0;
    stats.last_update = std::chrono::system_clock::now();
    
    for (const auto& [user_id, session] : online_users_) {
        if (session->connection_type == "websocket") {
            stats.websocket_connections++;
        } else {
            stats.http_sessions++;
        }
    }
    
    return stats;
}

void UserStatusManager::heartbeatCheckLoop()
{
    while (running_.load()) {
        cleanupTimeoutUsers();
        std::this_thread::sleep_for(CHECK_INTERVAL);
    }
}

void UserStatusManager::cleanupTimeoutUsers()
{
    auto now = std::chrono::system_clock::now();
    std::vector<std::string> timeout_users;
    
    {
        std::lock_guard<std::mutex> lock(status_mutex_);
        
        for (const auto& [user_id, session] : online_users_) {
            auto time_since_heartbeat = now - session->last_heartbeat;
            if (time_since_heartbeat > HEARTBEAT_TIMEOUT) {
                timeout_users.push_back(user_id);
            }
        }
    }
    
    // 清理超时用户
    for (const std::string& user_id : timeout_users) {
        LOG_WARN << "User " << user_id << " timed out, removing from online list";
        userLogout(user_id);
    }
    
    if (!timeout_users.empty()) {
        LOG_INFO << "Cleaned up " << timeout_users.size() << " timeout users";
    }
}

void UserStatusManager::updateUserOnlineStatusInDB(const std::string& user_id, bool is_online)
{
    try {
        // 这里可以调用数据库管理器的方法来更新用户在线状态
        // 目前数据库中可能没有相应的字段，可以在用户表中添加 is_online 字段
        LOG_DEBUG << "Updated user " << user_id << " online status to " << (is_online ? "online" : "offline") << " in database";
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to update user online status in database: " << e.what();
    }
}

void UserStatusManager::broadcastUserStatusChange(const std::string& user_id, bool is_online)
{
    // 这里可以通过WebSocket向相关用户广播状态变化
    // 例如向用户的好友或者同一房间的其他用户发送通知
    LOG_DEBUG << "User " << user_id << " status changed to " << (is_online ? "online" : "offline");
}
