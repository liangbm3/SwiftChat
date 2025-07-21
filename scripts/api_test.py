# e2e_test.py
import requests
import json
import uuid
import sys
import websocket
import time
from threading import Thread

# --- 配置 ---
BASE_URL = "http://localhost:8080"
WEBSOCKET_URL = "ws://localhost:8081"

# --- 全局状态 ---
test_state = {}
test_summary = {"passed": 0, "failed": 0}

# --- 辅助函数 ---
def print_test_header(name):
    print("\n" + "="*60)
    print(f"  🚀  {name}")
    print("="*60)

def print_pass(message):
    global test_summary
    test_summary["passed"] += 1
    print(f"  ✅ \033[92m[PASS]\033[0m {message}")

def print_fail(message, details=None):
    global test_summary
    test_summary["failed"] += 1
    print(f"  ❌ \033[91m[FAIL]\033[0m {message}")
    if details:
        print(f"      \033[90mDetails: {details}\033[0m")
    # 在E2E测试中，一旦失败就退出，因为后续步骤依赖于此
    sys.exit(1)
    
def ws_recv_type(ws, expected_type, timeout=5):
    """健壮地接收并验证特定类型的WebSocket消息"""
    start_time = time.time()
    while time.time() - start_time < timeout:
        try:
            raw_message = ws.recv()
            message = json.loads(raw_message)
            print(f"  [WS RECV] {ws.sock.getpeername()} | Type: {message.get('data', {}).get('type')}, Success: {message.get('success')}")
            if message.get("data", {}).get("type") == expected_type:
                return message
        except websocket.WebSocketTimeoutException:
            continue # 超时是正常的，继续等待
        except Exception as e:
            print_fail(f"处理WebSocket消息时出错: {e}", raw_message)
    print_fail(f"在 {timeout}s 内未接收到类型为 '{expected_type}' 的消息")

# --- 端到端测试主流程 ---

def run_e2e_test():
    """执行完整的端到端测试流程"""
    session_a = requests.Session()
    session_b = requests.Session()

    # --- 1. 准备阶段：HTTP API 创建用户和房间 ---
    print_test_header("E2E - 步骤 1: 环境准备 (注册用户, 创建房间)")
    
    # 注册用户A和B
    user_a_creds = {'username': f"e2e_user_a_{uuid.uuid4().hex[:8]}", 'password': 'password123'}
    user_b_creds = {'username': f"e2e_user_b_{uuid.uuid4().hex[:8]}", 'password': 'password456'}
    
    resp_a = session_a.post(f"{BASE_URL}/api/v1/auth/register", json=user_a_creds)
    if resp_a.status_code != 201: print_fail("注册用户A失败", resp_a.text)
    test_state['user_a'] = {**user_a_creds, **resp_a.json()['data']}
    print_pass("用户A注册并登录成功.")

    resp_b = session_b.post(f"{BASE_URL}/api/v1/auth/register", json=user_b_creds)
    if resp_b.status_code != 201: print_fail("注册用户B失败", resp_b.text)
    test_state['user_b'] = {**user_b_creds, **resp_b.json()['data']}
    print_pass("用户B注册并登录成功.")

    # 用户A创建房间
    session_a.headers.update({"Authorization": f"Bearer {test_state['user_a']['token']}"})
    room_data = {"name": f"E2E测试房-{uuid.uuid4().hex[:4]}", "description": "端到端测试"}
    resp_room = session_a.post(f"{BASE_URL}/api/v1/rooms", json=room_data)
    if resp_room.status_code != 201: print_fail("创建房间失败", resp_room.text)
    test_state['room'] = resp_room.json()['data']
    print_pass(f"房间 '{room_data['name']}' 创建成功.")

    # --- 2. WebSocket 交互测试 ---
    print_test_header("E2E - 步骤 2: WebSocket 实时交互")
    ws_a = None
    ws_b = None
    try:
        # 连接
        ws_a = websocket.create_connection(WEBSOCKET_URL, timeout=5)
        ws_b = websocket.create_connection(WEBSOCKET_URL, timeout=5)
        print_pass("WebSocket连接已建立 (用户A & B).")

        # 认证
        ws_a.send(json.dumps({"type": "auth", "token": test_state['user_a']['token']}))
        auth_a_resp = json.loads(ws_a.recv())
        if not (auth_a_resp.get("success") and auth_a_resp.get("data", {}).get("status") == "connected"):
            print_fail("用户A WebSocket认证失败", auth_a_resp)

        ws_b.send(json.dumps({"type": "auth", "token": test_state['user_b']['token']}))
        auth_b_resp = json.loads(ws_b.recv())
        if not (auth_b_resp.get("success") and auth_b_resp.get("data", {}).get("status") == "connected"):
            print_fail("用户B WebSocket认证失败", auth_b_resp)
        print_pass("WebSocket认证成功 (用户A & B).")

        # 加入房间并验证广播
        room_id = test_state['room']['id']
        ws_a.send(json.dumps({"type": "join_room", "room_id": room_id}))
        join_a_self = ws_recv_type(ws_a, "room_joined")
        if join_a_self['data']['user_id'] != test_state['user_a']['id']:
             print_fail("用户A加入房间响应不正确", join_a_self)
        print_pass("用户A成功加入房间.")

        ws_b.send(json.dumps({"type": "join_room", "room_id": room_id}))
        # 并行接收：B收到自己的加入确认，A收到B加入的广播
        join_b_self = ws_recv_type(ws_b, "room_joined")
        if join_b_self['data']['user_id'] != test_state['user_b']['id']:
             print_fail("用户B加入房间响应不正确", join_b_self)
        print_pass("用户B成功加入房间.")

        join_a_other = ws_recv_type(ws_a, "user_joined")
        if join_a_other['data']['user_id'] != test_state['user_b']['id']:
            print_fail("用户A未收到或收到的用户B加入广播不正确", join_a_other)
        print_pass("用户A收到了用户B加入的广播通知.")

        # 发送和接收消息 (严格按照文档进行验证)
        message_from_a = f"Hello from A! - {uuid.uuid4().hex[:4]}"
        ws_a.send(json.dumps({"type": "send_message", "content": message_from_a}))
        
        # 验证: A收到'message_sent'确认, B收到'message_received'广播
        sent_a = ws_recv_type(ws_a, "message_sent")
        if sent_a['data']['content'] != message_from_a: print_fail("用户A 'message_sent' 确认内容不匹配")
        
        recv_b = ws_recv_type(ws_b, "message_received")
        if not (recv_b['data']['content'] == message_from_a and recv_b['data']['user_id'] == test_state['user_a']['id']):
             print_fail("用户B收到的广播消息不正确", recv_b)
        print_pass("消息流程(A->B)测试通过: 'message_sent'和'message_received'均正确.")
        
        # 保存消息用于后续验证
        test_state['messages'] = [message_from_a]

    finally:
        if ws_a: ws_a.close()
        if ws_b: ws_b.close()
        print_pass("WebSocket连接已关闭.")

    # --- 3. 验证阶段：HTTP API 检查消息持久化 ---
    print_test_header("E2E - 步骤 3: 验证持久化和清理")
    
    # 验证消息历史
    resp_msg = session_a.get(f"{BASE_URL}/api/v1/messages", params={'room_id': test_state['room']['id']})
    if resp_msg.status_code != 200: print_fail("获取消息历史失败", resp_msg.text)
    messages = resp_msg.json()['data']['messages']
    if any(msg['content'] == test_state['messages'][0] for msg in messages):
        print_pass("GET /messages - 成功在历史记录中找到WebSocket发送的消息.")
    else:
        print_fail("消息持久化失败: 未在API响应中找到发送的消息.", messages)

    # 清理资源
    resp_del = session_a.delete(f"{BASE_URL}/api/v1/rooms/{test_state['room']['id']}")
    if resp_del.status_code != 200: print_fail("清理阶段：删除房间失败", resp_del.text)
    print_pass("资源清理：房间已成功删除.")

def main():
    try:
        run_e2e_test()
    except requests.exceptions.ConnectionError:
        print_fail(f"无法连接到服务器 {BASE_URL}. 请确保您的API服务器正在运行。")
    except Exception as e:
        print_fail(f"测试期间发生未知错误: {e}")
    finally:
        print("\n" + "="*60)
        print("📊  E2E测试总结")
        print(f"  通过: {test_summary['passed']}, 失败: {test_summary['failed']}")
        print("="*60)
        
        if test_summary["failed"] > 0:
            sys.exit(1)

if __name__ == "__main__":
    main()