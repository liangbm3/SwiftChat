#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <chrono>
#include <memory>
#include <atomic>
#include <thread>

// 前向声明
class DatabaseManager;

/**
 * 用户状态管理器
 * 负责维护用户的在线状态、心跳检测、会话管理等
 */
class UserStatusManager
{
public:
    struct UserSession
    {
        std::string user_id;
        std::chrono::system_clock::time_point last_heartbeat;
        std::chrono::system_clock::time_point login_time;
        std::string connection_type; // "websocket" 或 "http"
        void* connection_handle;     // WebSocket连接句柄或HTTP会话标识
        
        UserSession(const std::string& uid, const std::string& conn_type, void* handle = nullptr)
            : user_id(uid), connection_type(conn_type), connection_handle(handle)
        {
            auto now = std::chrono::system_clock::now();
            last_heartbeat = now;
            login_time = now;
        }
    };

private:
    DatabaseManager& db_manager_;
    
    // 用户在线状态映射：user_id -> UserSession
    std::unordered_map<std::string, std::shared_ptr<UserSession>> online_users_;
    
    // WebSocket连接映射：connection_handle -> user_id
    std::unordered_map<void*, std::string> websocket_connections_;
    
    // 线程安全锁
    mutable std::mutex status_mutex_;
    
    // 心跳检测线程
    std::thread heartbeat_thread_;
    std::atomic<bool> running_;
    
    // 配置参数
    static constexpr std::chrono::seconds HEARTBEAT_TIMEOUT{30}; // 30秒心跳超时
    static constexpr std::chrono::seconds CHECK_INTERVAL{10};    // 10秒检查间隔

public:
    explicit UserStatusManager(DatabaseManager& db_manager);
    ~UserStatusManager();

    // 禁用拷贝构造和赋值
    UserStatusManager(const UserStatusManager&) = delete;
    UserStatusManager& operator=(const UserStatusManager&) = delete;

    // 启动和停止服务
    void start();
    void stop();

    // 用户登录/登出
    void userLogin(const std::string& user_id, const std::string& connection_type, void* connection_handle = nullptr);
    void userLogout(const std::string& user_id);
    void userLogoutByConnection(void* connection_handle);

    // 心跳更新
    void updateHeartbeat(const std::string& user_id);
    void updateHeartbeatByConnection(void* connection_handle);

    // 状态查询
    bool isUserOnline(const std::string& user_id) const;
    std::vector<std::string> getOnlineUsers() const;
    std::vector<std::string> getOnlineUsersInRoom(const std::string& room_id) const;
    size_t getOnlineUserCount() const;
    
    // 获取用户会话信息
    std::shared_ptr<UserSession> getUserSession(const std::string& user_id) const;
    std::chrono::system_clock::time_point getLastActivity(const std::string& user_id) const;
    std::chrono::duration<double> getOnlineDuration(const std::string& user_id) const;

    // WebSocket连接管理
    void registerWebSocketConnection(const std::string& user_id, void* connection_handle);
    void unregisterWebSocketConnection(void* connection_handle);
    std::string getUserIdByConnection(void* connection_handle) const;

    // 获取在线状态统计
    struct OnlineStats
    {
        size_t total_online;
        size_t websocket_connections;
        size_t http_sessions;
        std::chrono::system_clock::time_point last_update;
    };
    OnlineStats getOnlineStats() const;

private:
    // 心跳检测线程函数
    void heartbeatCheckLoop();
    
    // 清理超时用户
    void cleanupTimeoutUsers();
    
    // 更新数据库中的用户在线状态
    void updateUserOnlineStatusInDB(const std::string& user_id, bool is_online);
    
    // 广播用户状态变化（可选，用于实时通知）
    void broadcastUserStatusChange(const std::string& user_id, bool is_online);
};
