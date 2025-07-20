import requests
import json
import uuid
import sys
import websocket

# --- 配置 ---
BASE_URL = "http://localhost:8080"
WEBSOCKET_URL = "ws://localhost:8081"

# --- 全局状态，用于在测试函数间传递数据 ---
test_state = {}
test_summary = {"passed": 0, "failed": 0}

# --- 辅助函数，用于美化输出 ---
def print_test_header(name):
    print("\n" + "="*50)
    print(f"  🧪  {name}")
    print("="*50)

def print_pass(message):
    global test_summary
    test_summary["passed"] += 1
    # 在支持颜色的终端中显示为绿色
    print(f"  ✅ \033[92m[PASS]\033[0m {message}")

def print_fail(message):
    global test_summary
    test_summary["failed"] += 1
    # 在支持颜色的终端中显示为红色
    print(f"  ❌ \033[91m[FAIL]\033[0m {message}")
    # 失败时立即退出，因为后续测试可能依赖于此
    sys.exit(1) 

def check_response(response, expected_status_code, check_data_keys=None):
    """通用响应检查器"""
    # 1. 检查状态码
    if response.status_code != expected_status_code:
        print_fail(f"期望状态码 {expected_status_code}, 但收到 {response.status_code}. 响应: {response.text}")
        return False
    
    # 2. 检查基础响应结构
    try:
        data = response.json()
    except json.JSONDecodeError:
        print_fail(f"响应不是有效的JSON格式. 响应: {response.text}")
        return False

    if "success" not in data or data["success"] is not True:
        print_fail(f"响应中 'success' 字段不为 true. 响应: {data}")
        return False

    if "data" not in data:
        print_fail(f"响应中缺少 'data' 字段. 响应: {data}")
        return False

    # 3. 检查data字段下的特定键
    if check_data_keys:
        for key in check_data_keys:
            if key not in data.get("data", {}):
                print_fail(f"'data' 字段中缺少key: '{key}'. 响应: {data}")
                return False
    
    return True


# --- 测试函数 ---

def test_01_system_health():
    print_test_header("GET /api/v1/health")
    response = requests.get(f"{BASE_URL}/api/v1/health")
    if check_response(response, 200, ["status", "timestamp"]):
        print_pass("服务器健康检查接口格式正确.")

def test_02_system_info():
    print_test_header("GET /api/v1/info")
    response = requests.get(f"{BASE_URL}/api/v1/info")
    if check_response(response, 200, ["name", "version"]):
        print_pass("服务器信息接口格式正确.")

def test_03_auth_register(session):
    print_test_header("POST /api/v1/auth/register")
    unique_user = f"tester_{uuid.uuid4().hex[:8]}"
    test_state['user_a'] = {'username': unique_user, 'password': 'password123'}
    
    response = session.post(f"{BASE_URL}/api/v1/auth/register", json=test_state['user_a'])
    if check_response(response, 201, ["token", "id", "username"]):
        test_state['user_a']['id'] = response.json()['data']['id']
        print_pass("用户注册接口格式正确.")

def test_04_auth_login(session):
    print_test_header("POST /api/v1/auth/login")
    response = session.post(f"{BASE_URL}/api/v1/auth/login", json=test_state['user_a'])
    if check_response(response, 200, ["token", "id", "username"]):
        token = response.json()['data']['token']
        test_state['user_a']['token'] = token
        session.headers.update({"Authorization": f"Bearer {token}"})
        print_pass("用户登录接口格式正确, Token已设置.")
        
def test_05_users_get_me(session):
    print_test_header("GET /api/v1/users/me")
    response = session.get(f"{BASE_URL}/api/v1/users/me")
    if check_response(response, 200, ["user"]):
        print_pass("获取当前用户信息接口格式正确.")

def test_06_users_get_list(session):
    print_test_header("GET /api/v1/users")
    response = session.get(f"{BASE_URL}/api/v1/users")
    if check_response(response, 200, ["users", "total", "limit", "offset"]):
        print_pass("获取用户列表接口格式正确.")

def test_07_users_get_specific(session):
    print_test_header("GET /api/v1/users/{user_id}")
    user_id = test_state['user_a']['id']
    response = session.get(f"{BASE_URL}/api/v1/users/{user_id}")
    if check_response(response, 200, ["user"]):
        print_pass("获取指定用户信息接口格式正确.")

def test_08_rooms_create(session):
    print_test_header("POST /api/v1/rooms")
    room_data = {
        "name": f"格式测试房间-{uuid.uuid4().hex[:4]}",
        "description": "用于测试API格式"
    }
    response = session.post(f"{BASE_URL}/api/v1/rooms", json=room_data)
    if check_response(response, 201, ["id", "name", "creator_id"]):
        test_state['room_id'] = response.json()['data']['id']
        print_pass("创建房间接口格式正确.")

def test_09_rooms_get_list(session):
    print_test_header("GET /api/v1/rooms")
    response = session.get(f"{BASE_URL}/api/v1/rooms")
    if check_response(response, 200, ["rooms", "total", "limit", "offset"]):
        print_pass("获取房间列表接口格式正确.")

def test_10_rooms_update(session):
    print_test_header("PATCH /api/v1/rooms/{room_id}")
    room_id = test_state['room_id']
    update_data = {"description": "更新后的格式测试描述"}
    response = session.patch(f"{BASE_URL}/api/v1/rooms/{room_id}", json=update_data)
    if check_response(response, 200, ["room_id", "new_description", "updated_by"]):
        print_pass("更新房间接口格式正确.")
        
def test_11_messages_get_list(session):
    print_test_header("GET /api/v1/messages")
    params = {'room_id': test_state['room_id']}
    response = session.get(f"{BASE_URL}/api/v1/messages", params=params)
    if check_response(response, 200, ["messages", "room_id", "count"]):
        print_pass("获取房间消息接口格式正确.")

def test_12_rooms_join_and_leave():
    print_test_header("POST /rooms/join and /rooms/leave")
    # 为用户B创建一个独立的会话
    session_b = requests.Session()
    user_b_creds = {'username': f"tester_{uuid.uuid4().hex[:8]}", 'password': 'password123'}

    # 注册和登录用户B
    reg_resp = session_b.post(f"{BASE_URL}/api/v1/auth/register", json=user_b_creds)
    if reg_resp.status_code != 201:
        print_fail("为加入/离开测试注册用户B失败")
        return

    log_resp = session_b.post(f"{BASE_URL}/api/v1/auth/login", json=user_b_creds)
    if log_resp.status_code != 200:
        print_fail("为加入/离开测试登录用户B失败")
        return
    token_b = log_resp.json()['data']['token']
    session_b.headers.update({"Authorization": f"Bearer {token_b}"})

    # 用户B加入房间
    join_payload = {"room_id": test_state['room_id']}
    join_resp = session_b.post(f"{BASE_URL}/api/v1/rooms/join", json=join_payload)
    if not check_response(join_resp, 200, ["room_id", "user_id", "joined_at"]):
        print_fail("加入房间接口格式不正确.")
        return
    print_pass("加入房间接口格式正确.")

    # 用户B离开房间
    leave_payload = {"room_id": test_state['room_id']}
    leave_resp = session_b.post(f"{BASE_URL}/api/v1/rooms/leave", json=leave_payload)
    if not check_response(leave_resp, 200, ["room_id", "user_id"]):
        print_fail("离开房间接口格式不正确.")
        return
    print_pass("离开房间接口格式正确.")

def test_13_websocket_auth():
    print_test_header("WebSocket Authentication")
    try:
        ws = websocket.create_connection(WEBSOCKET_URL, timeout=5)
        auth_payload = {"type": "auth", "token": test_state['user_a']['token']}
        ws.send(json.dumps(auth_payload))
        response_str = ws.recv()
        ws.close()

        response = json.loads(response_str)
        if response.get("success") is True and "user_id" in response.get("data", {}):
            print_pass("WebSocket认证消息格式正确.")
        else:
            print_fail(f"WebSocket认证响应格式不正确. 收到: {response_str}")

    except Exception as e:
        print_fail(f"连接或测试WebSocket时发生错误: {e}")

def test_99_rooms_delete(session):
    print_test_header("DELETE /api/v1/rooms/{room_id} (Cleanup)")
    room_id = test_state.get('room_id')
    if not room_id:
        print("🤔 [SKIP] 没有创建房间，跳过删除。")
        return
        
    response = session.delete(f"{BASE_URL}/api/v1/rooms/{room_id}")
    if check_response(response, 200, ["room_id", "deleted_by"]):
        print_pass("删除房间接口格式正确.")


def main():
    """主函数，按顺序执行所有测试"""
    session_a = requests.Session()
    
    try:
        # 无需认证的接口
        test_01_system_health()
        test_02_system_info()
        
        # 认证流程
        test_03_auth_register(session_a)
        test_04_auth_login(session_a)
        
        # 用户管理 (需要认证)
        test_05_users_get_me(session_a)
        test_06_users_get_list(session_a)
        test_07_users_get_specific(session_a)
        
        # 房间和消息管理
        test_08_rooms_create(session_a)
        test_09_rooms_get_list(session_a)
        test_10_rooms_update(session_a)
        test_11_messages_get_list(session_a)
        
        # 涉及第二个用户的交互
        test_12_rooms_join_and_leave()
        
        # WebSocket
        test_13_websocket_auth()

    except requests.exceptions.ConnectionError:
        print_fail(f"无法连接到服务器 {BASE_URL}. 请确保您的API服务器正在运行。")
    except Exception as e:
        print_fail(f"测试期间发生未知错误: {e}")
    finally:
        # 清理资源
        if 'token' in test_state.get('user_a', {}):
            test_99_rooms_delete(session_a)
        
        print("\n" + "="*50)
        print("📊  测试总结")
        print(f"  总计: {sum(test_summary.values())}, 通过: {test_summary['passed']}, 失败: {test_summary['failed']}")
        print("="*50)

if __name__ == "__main__":
    main()