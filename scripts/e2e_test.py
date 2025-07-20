import requests
import websocket
import json
import uuid
import time
import socket

# --- 配置 ---
BASE_URL = "http://localhost:8080"
WEBSOCKET_URL = "ws://localhost:8081"

class SwiftChatE2ETest:
    """
    一个完整的SwiftChat API端到端测试类
    """
    def __init__(self):
        self.session_a = requests.Session()
        self.session_b = requests.Session()
        self.user_a_creds = {}
        self.user_b_creds = {}
        self.room_id = None

    def _generate_user_credentials(self):
        """为每个测试运行生成唯一的用户名以避免冲突"""
        unique_id = str(uuid.uuid4())[:8]
        return {
            "username": f"user_{unique_id}",
            "password": "password123"
        }

    def _register(self, session, user_creds):
        """注册一个新用户"""
        print(f"[*] 正在注册用户: {user_creds['username']}...")
        response = session.post(f"{BASE_URL}/api/v1/auth/register", json=user_creds)
        assert response.status_code == 201, f"注册失败: {response.text}"
        data = response.json()
        assert data["success"] is True
        print(f"[+] 用户 '{data['data']['username']}' 注册成功.")
        return data["data"]

    def _login(self, session, user_creds):
        """用户登录并设置Authorization头"""
        print(f"[*] 正在登录用户: {user_creds['username']}...")
        response = session.post(f"{BASE_URL}/api/v1/auth/login", json=user_creds)
        assert response.status_code == 200, f"登录失败: {response.text}"
        data = response.json()
        assert data["success"] is True
        token = data["data"]["token"]
        session.headers.update({"Authorization": f"Bearer {token}"})
        print(f"[+] 用户 '{data['data']['username']}' 登录成功.")
        return token

    def _create_room(self, session):
        """用户A创建一个房间"""
        print("[*] 用户A正在创建房间...")
        room_data = {
            "name": f"测试房间-{str(uuid.uuid4())[:4]}",
            "description": "这是一个端到端测试创建的房间"
        }
        response = session.post(f"{BASE_URL}/api/v1/rooms", json=room_data)
        assert response.status_code == 201, f"创建房间失败: {response.text}"
        data = response.json()
        assert data["success"] is True
        self.room_id = data["data"]["id"]
        print(f"[+] 房间 '{data['data']['name']}' 创建成功, ID: {self.room_id}")

    def _update_room(self, session):
        """用户A更新房间描述"""
        print(f"[*] 用户A正在更新房间 {self.room_id} 的描述...")
        update_data = {"description": "这是更新后的描述信息"}
        response = session.patch(f"{BASE_URL}/api/v1/rooms/{self.room_id}", json=update_data)
        assert response.status_code == 200, f"更新房间失败: {response.text}"
        data = response.json()
        assert data["success"] is True
        assert data["data"]["new_description"] == update_data["description"]
        print("[+] 房间描述更新成功.")

    def _user_b_joins_room(self, session):
        """用户B加入房间"""
        print(f"[*] 用户B正在加入房间 {self.room_id}...")
        response = session.post(f"{BASE_URL}/api/v1/rooms/join", json={"room_id": self.room_id})
        assert response.status_code == 200, f"用户B加入房间失败: {response.text}"
        data = response.json()
        assert data["success"] is True
        print("[+] 用户B成功加入房间.")

    def _test_websocket_chat(self, token_a, token_b):
        """测试WebSocket实时消息收发"""
        print("[*] 正在测试WebSocket聊天功能...")
        
        # 为用户A和B建立WebSocket连接，设置超时
        ws_a = websocket.create_connection(WEBSOCKET_URL, timeout=10)
        ws_b = websocket.create_connection(WEBSOCKET_URL, timeout=10)
        print("[+] WebSocket连接已建立.")

        try:
            # 认证
            ws_a.send(json.dumps({"type": "auth", "token": token_a}))
            auth_resp_a = json.loads(ws_a.recv())
            assert auth_resp_a["success"], f"用户A WebSocket认证失败: {auth_resp_a}"

            ws_b.send(json.dumps({"type": "auth", "token": token_b}))
            auth_resp_b = json.loads(ws_b.recv())
            assert auth_resp_b["success"], f"用户B WebSocket认证失败: {auth_resp_b}"
            print("[+] WebSocket认证成功.")

            # 加入房间
            ws_a.send(json.dumps({"type": "join_room", "room_id": self.room_id}))
            join_resp_a = json.loads(ws_a.recv())
            assert join_resp_a["success"], f"用户A加入房间失败: {join_resp_a}"

            ws_b.send(json.dumps({"type": "join_room", "room_id": self.room_id}))
            join_resp_b = json.loads(ws_b.recv())
            assert join_resp_b["success"], f"用户B加入房间失败: {join_resp_b}"
            
            # 用户A可能会接收到用户B加入房间的通知，先读取并忽略
            try:
                user_joined_notification = json.loads(ws_a.recv())
                if user_joined_notification.get("data", {}).get("type") == "user_joined":
                    print("[*] 用户A收到用户B加入房间的通知（已忽略）")
            except:
                pass  # 如果没有通知就继续
                
            print("[+] WebSocket加入房间成功.")

            # 用户B发送消息
            chat_message = f"你好，我是B！这是测试消息 {str(uuid.uuid4())[:4]}"
            print(f"[*] 用户B正在发送消息: '{chat_message}'")
            ws_b.send(json.dumps({"type": "send_message", "content": chat_message}))
            
            # 用户B接收自己的消息确认
            print("[*] 用户B正在等待消息确认...")
            try:
                sent_confirm = json.loads(ws_b.recv())
                assert sent_confirm['success'] is True, f"发送确认失败: {sent_confirm}"
                assert sent_confirm['data']['type'] == 'message_received', f"确认消息类型不正确: {sent_confirm}"
                print("[+] 用户B收到发送确认.")
            except socket.timeout:
                raise AssertionError("用户B接收消息确认超时")

            # 用户A接收消息
            print("[*] 用户A正在等待接收消息...")
            try:
                received_message = json.loads(ws_a.recv())
                
                assert received_message["success"] is True, f"接收到的消息success不为true: {received_message}"
                msg_data = received_message["data"]
                assert msg_data["type"] == "message_received", f"消息类型不正确: {msg_data}"
                assert msg_data["content"] == chat_message, f"消息内容不匹配。期望: '{chat_message}', 实际: '{msg_data['content']}'"
                assert msg_data["room_id"] == self.room_id, f"房间ID不匹配。期望: '{self.room_id}', 实际: '{msg_data['room_id']}'"
                assert msg_data["user_id"] == self.user_b_creds["id"], f"发送者ID不匹配。期望: '{self.user_b_creds['id']}', 实际: '{msg_data['user_id']}'"
                
                print(f"[+] 用户A成功收到来自用户B的消息: '{msg_data['content']}'")
            except socket.timeout:
                raise AssertionError("用户A接收消息超时")

        finally:
            ws_a.close()
            ws_b.close()
            print("[+] WebSocket连接已关闭.")

    def _user_b_leaves_room(self, session):
        """用户B离开房间"""
        print(f"[*] 用户B正在离开房间 {self.room_id}...")
        response = session.post(f"{BASE_URL}/api/v1/rooms/leave", json={"room_id": self.room_id})
        assert response.status_code == 200, f"用户B离开房间失败: {response.text}"
        assert response.json()["success"] is True
        print("[+] 用户B成功离开房间.")

    def _delete_room(self, session):
        """用户A删除房间"""
        print(f"[*] 用户A正在删除房间 {self.room_id}...")
        response = session.delete(f"{BASE_URL}/api/v1/rooms/{self.room_id}")
        assert response.status_code == 200, f"删除房间失败: {response.text}"
        data = response.json()
        assert data["success"] is True
        assert data["data"]["room_id"] == self.room_id
        print("[+] 房间删除成功.")

    def _check_health(self):
        """检查系统健康API"""
        print("[*] 正在检查服务器健康状态...")
        response = requests.get(f"{BASE_URL}/api/v1/health")
        assert response.status_code == 200
        data = response.json()
        assert data['data']['status'] == 'ok'
        print("[+] 服务器健康状态正常.")

    def run(self):
        """执行完整的端到端测试流程"""
        print("--- 开始SwiftChat端到端测试 ---")
        try:
            # --- 用户A流程 ---
            self.user_a_creds = self._generate_user_credentials()
            user_a_data = self._register(self.session_a, self.user_a_creds)
            token_a = self._login(self.session_a, self.user_a_creds)
            self.user_a_creds['id'] = user_a_data['id']
            
            self._create_room(self.session_a)
            self._update_room(self.session_a)

            # --- 用户B流程 ---
            self.user_b_creds = self._generate_user_credentials()
            user_b_data = self._register(self.session_b, self.user_b_creds)
            token_b = self._login(self.session_b, self.user_b_creds)
            self.user_b_creds['id'] = user_b_data['id']

            self._user_b_joins_room(self.session_b)
            
            # --- WebSocket交互测试 ---
            self._test_websocket_chat(token_a, token_b)

            # --- 清理流程 ---
            self._user_b_leaves_room(self.session_b)
            self._delete_room(self.session_a)

            # --- 系统检查 ---
            self._check_health()
            
            print("\n✅ \033[92m所有端到端测试均已成功通过！\033[0m")

        except AssertionError as e:
            print(f"\n❌ \033[91m测试失败: {e}\033[0m")
        except Exception as e:
            print(f"\n❌ \033[91m测试期间发生意外错误: {e}\033[0m")


if __name__ == "__main__":
    test_runner = SwiftChatE2ETest()
    test_runner.run()