class SwiftChatApp {
    constructor() {
        this.currentUser = null;
        this.currentRoom = null;
        this.authToken = localStorage.getItem('authToken');
        this.websocket = null;
        this.rooms = [];
        this.messages = [];
        
        this.initializeApp();
        this.setupEventListeners();
    }

    initializeApp() {
        // 请求通知权限
        this.checkNotificationPermission();
        
        // 如果有存储的token，尝试自动登录
        if (this.authToken) {
            this.validateToken();
        } else {
            this.showAuthContainer();
        }
    }

    setupEventListeners() {
        // 认证表单事件
        document.getElementById('login-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleLogin();
        });

        document.getElementById('register-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleRegister();
        });

        // 聊天相关事件
        document.getElementById('logout-btn').addEventListener('click', () => {
            this.logout();
        });

        document.getElementById('create-room-btn').addEventListener('click', () => {
            this.showCreateRoomModal();
        });

        document.getElementById('create-room-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleCreateRoom();
        });

        document.getElementById('leave-room-btn').addEventListener('click', () => {
            this.leaveCurrentRoom();
        });

        document.getElementById('message-input').addEventListener('keypress', (e) => {
            if (e.key === 'Enter') {
                this.sendMessage();
            }
        });

        document.getElementById('send-btn').addEventListener('click', () => {
            this.sendMessage();
        });

        // 模态框关闭事件
        document.querySelector('.modal-close').addEventListener('click', () => {
            this.closeCreateRoomModal();
        });

        // 点击模态框背景关闭
        document.getElementById('create-room-modal').addEventListener('click', (e) => {
            if (e.target.id === 'create-room-modal') {
                this.closeCreateRoomModal();
            }
        });
    }

    // 认证相关方法
    async validateToken() {
        try {
            this.showLoading('验证登录状态...');
            
            // 检查token是否有效
            if (!this.authToken || this.authToken === 'undefined' || this.authToken === 'null') {
                throw new Error('Invalid token');
            }
            
            const response = await fetch('/api/protected', {
                headers: {
                    'Authorization': `Bearer ${this.authToken}`
                }
            });

            if (response.ok) {
                // Token有效，获取用户信息
                const userData = this.parseJWT(this.authToken);
                this.currentUser = userData;
                this.showChatContainer();
                this.connectWebSocket();
                this.loadRooms();
            } else {
                // Token无效，清除并显示登录界面
                localStorage.removeItem('authToken');
                this.authToken = null;
                this.showAuthContainer();
            }
        } catch (error) {
            console.error('Token validation error:', error);
            this.showAuthContainer();
        } finally {
            this.hideLoading();
        }
    }

    async handleLogin() {
        const username = document.getElementById('login-username').value.trim();
        const password = document.getElementById('login-password').value;

        if (!username || !password) {
            this.showAuthMessage('请填写所有字段', 'error');
            return;
        }

        try {
            this.showLoading('登录中...');
            const response = await fetch('/api/v1/auth/login', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ username, password })
            });

            const data = await response.json();

            if (response.ok) {
                this.authToken = data.data.token; // 修复：token在data.data.token路径下
                localStorage.setItem('authToken', this.authToken);
                this.currentUser = this.parseJWT(this.authToken);
                this.showAuthMessage('登录成功！', 'success');
                
                setTimeout(() => {
                    this.showChatContainer();
                    this.connectWebSocket();
                    this.loadRooms();
                }, 1000);
            } else {
                this.showAuthMessage(data.message || '登录失败', 'error');
            }
        } catch (error) {
            console.error('Login error:', error);
            this.showAuthMessage('网络错误，请稍后重试', 'error');
        } finally {
            this.hideLoading();
        }
    }

    async handleRegister() {
        const username = document.getElementById('register-username').value.trim();
        const password = document.getElementById('register-password').value;
        const confirmPassword = document.getElementById('register-confirm').value;

        if (!username || !password || !confirmPassword) {
            this.showAuthMessage('请填写所有字段', 'error');
            return;
        }

        if (password !== confirmPassword) {
            this.showAuthMessage('两次输入的密码不一致', 'error');
            return;
        }

        if (password.length < 6) {
            this.showAuthMessage('密码长度至少6位', 'error');
            return;
        }

        try {
            this.showLoading('注册中...');
            const response = await fetch('/api/v1/auth/register', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ username, password })
            });

            const data = await response.json();

            if (response.ok) {
                this.showAuthMessage('注册成功！请登录', 'success');
                this.showLogin();
                // 清空注册表单
                document.getElementById('register-form').reset();
            } else {
                this.showAuthMessage(data.message || '注册失败', 'error');
            }
        } catch (error) {
            console.error('Register error:', error);
            this.showAuthMessage('网络错误，请稍后重试', 'error');
        } finally {
            this.hideLoading();
        }
    }

    logout() {
        // 清除本地存储
        localStorage.removeItem('authToken');
        this.authToken = null;
        this.currentUser = null;
        this.currentRoom = null;
        this.rooms = [];
        this.messages = [];

        // 断开WebSocket连接
        if (this.websocket) {
            this.websocket.close();
            this.websocket = null;
        }

        // 显示登录界面
        this.showAuthContainer();
        this.clearForms();
    }

    // WebSocket相关方法
    connectWebSocket() {
        if (this.websocket) {
            this.websocket.close();
        }

        this.updateConnectionStatus('connecting');

        const wsUrl = `ws://${window.location.hostname}:8081`;
        this.websocket = new WebSocket(wsUrl);

        this.websocket.onopen = () => {
            console.log('WebSocket连接已建立');
            this.updateConnectionStatus('connected');
            // 发送认证消息
            const userInfo = this.parseJWT(this.authToken);
            this.sendWebSocketMessage({
                type: 'auth',
                token: this.authToken,
                user_id: userInfo.sub // JWT中的subject通常包含用户ID
            });
        };

        this.websocket.onmessage = (event) => {
            try {
                const message = JSON.parse(event.data);
                this.handleWebSocketMessage(message);
            } catch (error) {
                console.error('解析WebSocket消息错误:', error);
            }
        };

        this.websocket.onclose = () => {
            console.log('WebSocket连接已关闭');
            this.updateConnectionStatus('disconnected');
            // 如果用户仍然登录，尝试重连
            if (this.currentUser) {
                setTimeout(() => {
                    console.log('尝试重新连接WebSocket...');
                    this.connectWebSocket();
                }, 3000);
            }
        };

        this.websocket.onerror = (error) => {
            console.error('WebSocket错误:', error);
            this.updateConnectionStatus('disconnected');
        };
    }

    sendWebSocketMessage(message) {
        if (this.websocket && this.websocket.readyState === WebSocket.OPEN) {
            this.websocket.send(JSON.stringify(message));
        }
    }

    handleWebSocketMessage(message) {
        console.log('收到WebSocket消息:', message);

        switch (message.type) {
            case 'auth_response':
                if (message.success) {
                    console.log('WebSocket认证成功');
                } else {
                    console.error('WebSocket认证失败:', message.message);
                }
                break;

            case 'chat_message':
                this.displayMessage(message);
                break;

            case 'user_joined':
                this.showSystemMessage(`${message.username} 加入了房间`);
                break;

            case 'user_left':
                this.showSystemMessage(`${message.username} 离开了房间`);
                break;

            case 'room_created':
                this.loadRooms(); // 重新加载房间列表
                break;

            case 'error':
                this.showAuthMessage(message.message, 'error');
                break;

            default:
                console.log('未知的WebSocket消息类型:', message.type);
        }
    }

    // 房间相关方法
    async loadRooms() {
        try {
            const response = await fetch('/api/v1/rooms');
            
            if (response.ok) {
                const data = await response.json();
                // 修复：后端返回的是 data.data.rooms
                this.rooms = (data.data && data.data.rooms) || [];
                this.displayRooms();
            } else {
                const errorText = await response.text();
                console.error('加载房间列表失败:', response.status, errorText);
            }
        } catch (error) {
            console.error('加载房间列表错误:', error);
        }
    }

    displayRooms() {
        const roomList = document.getElementById('room-list');
        roomList.innerHTML = '';

        if (this.rooms.length === 0) {
            roomList.innerHTML = '<div class="no-rooms">暂无房间，创建一个开始聊天吧！</div>';
            return;
        }

        this.rooms.forEach(room => {
            const roomElement = this.createRoomElement(room);
            roomList.appendChild(roomElement);
        });
    }

    createRoomElement(room) {
        const roomDiv = document.createElement('div');
        roomDiv.className = 'room-item';
        roomDiv.dataset.roomId = room.id;

        roomDiv.innerHTML = `
            <div class="room-name">${this.escapeHtml(room.name)}</div>
            <div class="room-info">${room.member_count || 0} 成员</div>
        `;

        roomDiv.addEventListener('click', () => {
            console.log('点击房间:', room);
            this.joinRoom(room);
        });

        return roomDiv;
    }

    async joinRoom(room) {
        console.log('尝试加入房间:', room);
        console.log('当前房间:', this.currentRoom);
        
        if (this.currentRoom && this.currentRoom.id === room.id) {
            console.log('已经在这个房间了');
            return; // 已经在这个房间了
        }

        if (!this.authToken || this.authToken === 'undefined' || this.authToken === 'null') {
            this.showAuthMessage('认证token缺失，请重新登录', 'error');
            this.logout();
            return;
        }

        try {
            this.showLoading('加入房间中...');

            const response = await fetch('/api/v1/rooms/join', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'Authorization': `Bearer ${this.authToken}`
                },
                body: JSON.stringify({ room_id: room.id })
            });

            if (response.ok) {
                // 如果之前在其他房间，先离开
                if (this.currentRoom) {
                    this.sendWebSocketMessage({
                        type: 'leave_room',
                        room_id: this.currentRoom.id
                    });
                }

                this.currentRoom = room;
                this.updateRoomSelection();
                this.showChatRoom();
                this.loadMessages();

                // 通过WebSocket加入房间
                this.sendWebSocketMessage({
                    type: 'join_room',
                    room_id: room.id
                });
            } else {
                const data = await response.json();
                this.showAuthMessage(data.message || '加入房间失败', 'error');
            }
        } catch (error) {
            console.error('加入房间错误:', error);
            this.showAuthMessage('网络错误，请稍后重试', 'error');
        } finally {
            this.hideLoading();
        }
    }

    async handleCreateRoom() {
        const roomName = document.getElementById('room-name').value.trim();

        if (!roomName) {
            this.showAuthMessage('请输入房间名称', 'error');
            return;
        }

        if (!this.authToken || this.authToken === 'undefined' || this.authToken === 'null') {
            this.showAuthMessage('认证token缺失，请重新登录', 'error');
            this.logout();
            return;
        }

        try {
            this.showLoading('创建房间中...');

            const response = await fetch('/api/v1/rooms', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'Authorization': `Bearer ${this.authToken}`
                },
                body: JSON.stringify({ name: roomName })
            });

            const data = await response.json();

            if (response.ok) {
                this.closeCreateRoomModal();
                document.getElementById('create-room-form').reset();
                this.loadRooms();
                
                // 自动加入新创建的房间
                if (data.room) {
                    setTimeout(() => {
                        this.joinRoom(data.room);
                    }, 500);
                }
            } else {
                this.showAuthMessage(data.message || '创建房间失败', 'error');
            }
        } catch (error) {
            console.error('创建房间错误:', error);
            this.showAuthMessage('网络错误，请稍后重试', 'error');
        } finally {
            this.hideLoading();
        }
    }

    leaveCurrentRoom() {
        if (!this.currentRoom) return;

        // 通过WebSocket离开房间
        this.sendWebSocketMessage({
            type: 'leave_room',
            room_id: this.currentRoom.id
        });

        this.currentRoom = null;
        this.messages = [];
        this.updateRoomSelection();
        this.showNoRoomSelected();
    }

    // 消息相关方法
    async loadMessages() {
        if (!this.currentRoom) return;

        try {
            const response = await fetch(`/api/v1/messages?room_id=${this.currentRoom.id}`, {
                headers: {
                    'Authorization': `Bearer ${this.authToken}`
                }
            });

            if (response.ok) {
                const data = await response.json();
                this.messages = data.messages || [];
                this.displayMessages();
            } else {
                console.error('加载消息失败');
            }
        } catch (error) {
            console.error('加载消息错误:', error);
        }
    }

    displayMessages() {
        const messagesContainer = document.getElementById('messages-container');
        messagesContainer.innerHTML = '';

        if (this.messages.length === 0) {
            messagesContainer.innerHTML = '<div class="no-messages">暂无消息，发送第一条消息开始聊天吧！</div>';
            return;
        }

        this.messages.forEach(message => {
            const messageElement = this.createMessageElement(message);
            messagesContainer.appendChild(messageElement);
        });

        // 滚动到底部
        messagesContainer.scrollTop = messagesContainer.scrollHeight;
    }

    createMessageElement(message) {
        const messageDiv = document.createElement('div');
        messageDiv.className = 'message';
        
        if (message.sender === this.currentUser.username) {
            messageDiv.classList.add('own');
        }

        const avatarInitial = message.sender ? message.sender.charAt(0).toUpperCase() : '?';
        const timestamp = this.formatTimestamp(message.timestamp);

        messageDiv.innerHTML = `
            <div class="message-avatar">${avatarInitial}</div>
            <div class="message-content">
                <div class="message-header">
                    <span class="message-sender">${this.escapeHtml(message.sender)}</span>
                    <span class="message-time">${timestamp}</span>
                </div>
                <div class="message-text">${this.escapeHtml(message.content)}</div>
            </div>
        `;

        return messageDiv;
    }

    displayMessage(message) {
        // 添加到消息列表
        this.messages.push(message);

        // 如果消息属于当前房间，显示出来
        if (this.currentRoom && message.room_id === this.currentRoom.id) {
            const messagesContainer = document.getElementById('messages-container');
            const messageElement = this.createMessageElement(message);
            messagesContainer.appendChild(messageElement);
            messagesContainer.scrollTop = messagesContainer.scrollHeight;

            // 如果不是自己发送的消息，播放通知
            if (message.sender !== this.currentUser.username) {
                this.playNotificationSound();
                
                // 如果页面不在焦点，显示桌面通知
                if (document.hidden) {
                    this.showDesktopNotification(
                        `${message.sender} - ${this.currentRoom.name}`,
                        message.content
                    );
                }
            }
        }
    }

    sendMessage() {
        const messageInput = document.getElementById('message-input');
        const content = messageInput.value.trim();

        if (!content || !this.currentRoom) return;

        // 通过WebSocket发送消息
        this.sendWebSocketMessage({
            type: 'chat_message',
            room_id: this.currentRoom.id,
            content: content
        });

        // 清空输入框
        messageInput.value = '';
    }

    showSystemMessage(text) {
        if (!this.currentRoom) return;

        const systemMessage = {
            type: 'system',
            content: text,
            timestamp: new Date().toISOString(),
            sender: 'System'
        };

        const messagesContainer = document.getElementById('messages-container');
        const messageElement = this.createSystemMessageElement(systemMessage);
        messagesContainer.appendChild(messageElement);
        messagesContainer.scrollTop = messagesContainer.scrollHeight;
    }

    createSystemMessageElement(message) {
        const messageDiv = document.createElement('div');
        messageDiv.className = 'message system-message';
        messageDiv.innerHTML = `
            <div class="system-text">${this.escapeHtml(message.content)}</div>
        `;
        return messageDiv;
    }

    // UI 控制方法
    showAuthContainer() {
        document.getElementById('auth-container').classList.remove('hidden');
        document.getElementById('chat-container').classList.add('hidden');
    }

    showChatContainer() {
        document.getElementById('auth-container').classList.add('hidden');
        document.getElementById('chat-container').classList.remove('hidden');
        
        if (this.currentUser) {
            document.getElementById('current-username').textContent = this.currentUser.username;
        }
    }

    showLogin() {
        document.getElementById('login-form').classList.remove('hidden');
        document.getElementById('register-form').classList.add('hidden');
        
        document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));
        document.querySelector('.tab-btn').classList.add('active');
        
        this.clearAuthMessage();
    }

    showRegister() {
        document.getElementById('login-form').classList.add('hidden');
        document.getElementById('register-form').classList.remove('hidden');
        
        document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));
        document.querySelectorAll('.tab-btn')[1].classList.add('active');
        
        this.clearAuthMessage();
    }

    showNoRoomSelected() {
        document.getElementById('no-room-selected').classList.remove('hidden');
        document.getElementById('chat-room').classList.add('hidden');
    }

    showChatRoom() {
        document.getElementById('no-room-selected').classList.add('hidden');
        document.getElementById('chat-room').classList.remove('hidden');
        
        if (this.currentRoom) {
            document.getElementById('current-room-name').textContent = this.currentRoom.name;
        }
    }

    showCreateRoomModal() {
        document.getElementById('create-room-modal').classList.remove('hidden');
    }

    closeCreateRoomModal() {
        document.getElementById('create-room-modal').classList.add('hidden');
        document.getElementById('create-room-form').reset();
        this.clearAuthMessage();
    }

    showLoading(message = '加载中...') {
        const loading = document.getElementById('loading');
        loading.querySelector('p').textContent = message;
        loading.classList.remove('hidden');
    }

    hideLoading() {
        document.getElementById('loading').classList.add('hidden');
    }

    updateRoomSelection() {
        document.querySelectorAll('.room-item').forEach(item => {
            item.classList.remove('active');
        });

        if (this.currentRoom) {
            const currentRoomElement = document.querySelector(`[data-room-id="${this.currentRoom.id}"]`);
            if (currentRoomElement) {
                currentRoomElement.classList.add('active');
            }
        }
    }

    // 工具方法
    showAuthMessage(message, type = 'info') {
        const messageElement = document.getElementById('auth-message');
        messageElement.textContent = message;
        messageElement.className = `message ${type}`;
        
        // 3秒后自动清除消息
        setTimeout(() => {
            this.clearAuthMessage();
        }, 3000);
    }

    clearAuthMessage() {
        const messageElement = document.getElementById('auth-message');
        messageElement.textContent = '';
        messageElement.className = 'message';
    }

    clearForms() {
        document.getElementById('login-form').reset();
        document.getElementById('register-form').reset();
        document.getElementById('create-room-form').reset();
        this.clearAuthMessage();
    }

    parseJWT(token) {
        try {
            const base64Url = token.split('.')[1];
            const base64 = base64Url.replace(/-/g, '+').replace(/_/g, '/');
            const jsonPayload = decodeURIComponent(atob(base64).split('').map(function(c) {
                return '%' + ('00' + c.charCodeAt(0).toString(16)).slice(-2);
            }).join(''));
            
            return JSON.parse(jsonPayload);
        } catch (error) {
            console.error('解析JWT错误:', error);
            return null;
        }
    }

    formatTimestamp(timestamp) {
        const date = new Date(timestamp);
        const now = new Date();
        const diff = now - date;

        if (diff < 60000) { // 小于1分钟
            return '刚刚';
        } else if (diff < 3600000) { // 小于1小时
            return `${Math.floor(diff / 60000)}分钟前`;
        } else if (diff < 86400000) { // 小于1天
            return `${Math.floor(diff / 3600000)}小时前`;
        } else {
            return date.toLocaleDateString('zh-CN') + ' ' + date.toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit' });
        }
    }

    escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }

    updateConnectionStatus(status) {
        const statusElement = document.getElementById('connection-status');
        const textElement = document.getElementById('connection-text');
        
        statusElement.className = `connection-status ${status}`;
        
        switch (status) {
            case 'connected':
                textElement.textContent = '已连接';
                statusElement.classList.remove('hidden');
                setTimeout(() => {
                    statusElement.classList.add('hidden');
                }, 3000);
                break;
            case 'connecting':
                textElement.textContent = '连接中...';
                statusElement.classList.remove('hidden');
                break;
            case 'disconnected':
                textElement.textContent = '连接断开';
                statusElement.classList.remove('hidden');
                break;
        }
    }

    // 添加消息发送状态
    markMessageAsSending(messageElement) {
        messageElement.classList.add('message-sending');
    }

    markMessageAsSent(messageElement) {
        messageElement.classList.remove('message-sending');
    }

    // 添加声音通知（可选）
    playNotificationSound() {
        try {
            // 创建一个简单的音频通知
            const audioContext = new (window.AudioContext || window.webkitAudioContext)();
            const oscillator = audioContext.createOscillator();
            const gainNode = audioContext.createGain();
            
            oscillator.connect(gainNode);
            gainNode.connect(audioContext.destination);
            
            oscillator.frequency.value = 800;
            oscillator.type = 'sine';
            
            gainNode.gain.setValueAtTime(0, audioContext.currentTime);
            gainNode.gain.linearRampToValueAtTime(0.1, audioContext.currentTime + 0.01);
            gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.5);
            
            oscillator.start(audioContext.currentTime);
            oscillator.stop(audioContext.currentTime + 0.5);
        } catch (error) {
            // 如果音频API不可用，忽略错误
            console.log('音频通知不可用');
        }
    }

    // 检查浏览器通知权限
    checkNotificationPermission() {
        if ('Notification' in window) {
            if (Notification.permission === 'default') {
                Notification.requestPermission();
            }
        }
    }

    // 显示桌面通知
    showDesktopNotification(title, body) {
        if ('Notification' in window && Notification.permission === 'granted') {
            new Notification(title, {
                body: body,
                icon: '/favicon.ico',
                tag: 'swiftchat-message'
            });
        }
    }
}

// 全局函数（由HTML调用）
function showLogin() {
    app.showLogin();
}

function showRegister() {
    app.showRegister();
}

function closeCreateRoomModal() {
    app.closeCreateRoomModal();
}

// 当页面加载完成时启动应用
document.addEventListener('DOMContentLoaded', () => {
    window.app = new SwiftChatApp();
});
