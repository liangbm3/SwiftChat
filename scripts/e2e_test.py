# api_test.py
import requests
import json
import uuid
import sys

# --- 配置 ---
BASE_URL = "http://localhost:8080"

# --- 全局状态，用于在测试函数间传递数据 ---
test_state = {}
test_summary = {"passed": 0, "failed": 0}

# --- 辅助函数，用于美化输出 ---
def print_test_header(name):
    print("\n" + "="*60)
    print(f"  🧪  {name}")
    print("="*60)

def print_pass(message):
    global test_summary
    test_summary["passed"] += 1
    print(f"  ✅ \033[92m[PASS]\033[0m {message}")

def print_fail(message, response_text=None):
    global test_summary
    test_summary["failed"] += 1
    print(f"  ❌ \033[91m[FAIL]\033[0m {message}")
    if response_text:
        print(f"      \033[90mResponse: {response_text[:200]}...\033[0m")
    sys.exit(1)

def check_response(response, expected_status_code, check_data_keys=None, value_checks=None, expect_success=True):
    """通用响应检查器"""
    if response.status_code != expected_status_code:
        print_fail(f"期望状态码 {expected_status_code}, 但收到 {response.status_code}.", response.text)
    
    try:
        data = response.json()
    except json.JSONDecodeError:
        print_fail(f"响应不是有效的JSON格式.", response.text)

    if "success" not in data or data["success"] is not expect_success:
        print_fail(f"期望 'success' 字段为 {expect_success}, 但收到 {data.get('success')}.", data)

    if expect_success:
        if check_data_keys and "data" not in data:
            print_fail(f"响应中缺少 'data' 字段.", data)
        if check_data_keys:
            for key in check_data_keys:
                if key not in data.get("data", {}):
                    print_fail(f"'data' 字段中缺少key: '{key}'.", data)
        if value_checks:
            for path, expected_value in value_checks.items():
                keys = path.split('.')
                current_val = data
                try:
                    for key in keys:
                        current_val = current_val[key]
                    if current_val != expected_value:
                        print_fail(f"值检查失败. 路径 '{path}' 的值是 '{current_val}', 但期望是 '{expected_value}'.")
                except (KeyError, TypeError):
                    print_fail(f"值检查失败. 无法在响应中找到路径: '{path}'.")
    return True

# --- 测试函数 ---

def test_01_system_apis():
    print_test_header("1. 系统API测试 (Health, Info, Echo)")
    # Health Check
    response_health = requests.get(f"{BASE_URL}/api/v1/health")
    if check_response(response_health, 200, ["status"], {"data.status": "ok"}):
        print_pass("GET /health - 健康检查接口工作正常.")
    # Info Check
    response_info = requests.get(f"{BASE_URL}/api/v1/info")
    if check_response(response_info, 200, ["name", "version"]):
        print_pass("GET /info - 服务器信息接口格式正确.")
    # Echo Check
    response_echo = requests.get(f"{BASE_URL}/api/v1/echo")
    if check_response(response_echo, 200, ["method"], {"data.method": "GET"}):
        print_pass("GET /echo - Echo GET 测试通过.")

def test_02_auth_flow(session_a, session_b):
    print_test_header("2. 认证流程测试 (注册, 登录, 冲突, 失败)")
    # 准备用户数据
    user_a_creds = {'username': f"api_tester_a_{uuid.uuid4().hex[:8]}", 'password': 'password123'}
    user_b_creds = {'username': f"api_tester_b_{uuid.uuid4().hex[:8]}", 'password': 'password456'}
    test_state['user_a'] = user_a_creds
    test_state['user_b'] = user_b_creds
    
    # 用户A注册成功
    response_reg_a = session_a.post(f"{BASE_URL}/api/v1/auth/register", json=user_a_creds)
    if check_response(response_reg_a, 201, ["token", "id", "username"], {"data.username": user_a_creds['username']}):
        test_state['user_a'].update(response_reg_a.json()['data'])
        print_pass("POST /register - 用户A注册成功.")

    # 用户A注册冲突
    response_reg_conflict = session_a.post(f"{BASE_URL}/api/v1/auth/register", json=user_a_creds)
    if check_response(response_reg_conflict, 500, ["error"], expect_success=False):
        print_pass("POST /register - 用户名冲突（预期行为）测试通过.")

    # 登录失败
    response_login_fail = session_a.post(f"{BASE_URL}/api/v1/auth/login", json={'username': user_a_creds['username'], 'password': 'wrongpassword'})
    if check_response(response_login_fail, 401, ["error"], expect_success=False):
        print_pass("POST /login - 凭证错误（预期行为）测试通过.")
        
    # 登录成功
    response_login_ok = session_a.post(f"{BASE_URL}/api/v1/auth/login", json=user_a_creds)
    if check_response(response_login_ok, 200, ["token", "id", "username"]):
        token_a = response_login_ok.json()['data']['token']
        test_state['user_a']['token'] = token_a
        session_a.headers.update({"Authorization": f"Bearer {token_a}"})
        print_pass("POST /login - 用户A登录成功, Token已设置.")
        
    # 注册并登录用户B
    response_reg_b = session_b.post(f"{BASE_URL}/api/v1/auth/register", json=user_b_creds)
    test_state['user_b'].update(response_reg_b.json()['data'])
    token_b = response_reg_b.json()['data']['token']
    session_b.headers.update({"Authorization": f"Bearer {token_b}"})
    print_pass("POST /register & login - 用户B准备完成.")

def test_03_user_management(session_a):
    print_test_header("3. 用户管理API测试 (me, list, specific)")
    # 获取自己的信息
    response_me = session_a.get(f"{BASE_URL}/api/v1/users/me")
    if check_response(response_me, 200, ["user"], {"data.user.id": test_state['user_a']['id']}):
        print_pass("GET /users/me - 成功获取当前用户信息.")
        
    # 获取用户列表（带分页）
    response_list = session_a.get(f"{BASE_URL}/api/v1/users", params={"limit": 5, "offset": 0})
    if check_response(response_list, 200, ["users", "total", "limit", "offset"]):
        users = response_list.json()['data']['users']
        user_ids = [user['id'] for user in users]
        if test_state['user_a']['id'] in user_ids and test_state['user_b']['id'] in user_ids:
            print_pass("GET /users - 获取用户列表成功, 且包含测试用户.")
        else:
            print_fail("GET /users - 用户列表中未找到所有测试用户.")
            
    # 获取指定用户信息
    user_b_id = test_state['user_b']['id']
    response_specific = session_a.get(f"{BASE_URL}/api/v1/users/{user_b_id}")
    if check_response(response_specific, 200, ["user"], {"data.user.id": user_b_id}):
        print_pass("GET /users/{id} - 成功获取指定用户信息.")

def test_04_room_management(session_a, session_b):
    print_test_header("4. 房间管理API测试 (create, list, join, leave, patch, delete)")
    # 用户A创建房间
    room_data = {"name": f"API测试专用房-{uuid.uuid4().hex[:4]}", "description": "Desc 1"}
    response_create = session_a.post(f"{BASE_URL}/api/v1/rooms", json=room_data)
    if check_response(response_create, 201, ["id", "creator_id"], {"data.creator_id": test_state['user_a']['id']}):
        test_state['room'] = response_create.json()['data']
        print_pass("POST /rooms - 用户A创建房间成功.")

    # 获取房间列表
    response_list = session_a.get(f"{BASE_URL}/api/v1/rooms")
    if check_response(response_list, 200, ["rooms", "total"]):
        if any(r['id'] == test_state['room']['id'] for r in response_list.json()['data']['rooms']):
            print_pass("GET /rooms - 房间列表中包含新创建的房间.")
        else:
            print_fail("GET /rooms - 房间列表中找不到新房间.")

    # 用户B加入房间
    room_id = test_state['room']['id']
    response_join = session_b.post(f"{BASE_URL}/api/v1/rooms/join", json={"room_id": room_id})
    if check_response(response_join, 200, ["room_id"], {"data.room_id": room_id, "data.user_id": test_state['user_b']['id']}):
        print_pass("POST /rooms/join - 用户B加入房间成功.")

    # 用户B离开房间
    response_leave = session_b.post(f"{BASE_URL}/api/v1/rooms/leave", json={"room_id": room_id})
    if check_response(response_leave, 200, ["room_id"], {"data.room_id": room_id, "data.user_id": test_state['user_b']['id']}):
        print_pass("POST /rooms/leave - 用户B离开房间成功.")

    # 用户B（非创建者）更新房间失败
    update_data = {"description": "尝试非法更新"}
    response_patch_fail = session_b.patch(f"{BASE_URL}/api/v1/rooms/{room_id}", json=update_data)
    if check_response(response_patch_fail, 403, expect_success=False):
        print_pass("PATCH /rooms/{id} - 非创建者更新房间被拒绝（预期行为）.")

    # 用户A（创建者）更新房间成功
    update_data_ok = {"description": "更新成功"}
    response_patch_ok = session_a.patch(f"{BASE_URL}/api/v1/rooms/{room_id}", json=update_data_ok)
    if check_response(response_patch_ok, 200, ["new_description"], {"data.new_description": update_data_ok['description']}):
        print_pass("PATCH /rooms/{id} - 创建者更新房间成功.")

def test_05_message_management(session_a):
    print_test_header("5. 消息管理API测试")
    # 获取空房间的消息历史
    room_id = test_state['room']['id']
    response = session_a.get(f"{BASE_URL}/api/v1/messages", params={"room_id": room_id})
    if check_response(response, 200, ["messages", "count"], {"data.count": 0}):
        print_pass("GET /messages - 成功获取新房间的空消息列表.")

def test_99_cleanup(session_a, session_b):
    print_test_header("99. 资源清理")
    room_id = test_state.get('room', {}).get('id')
    if not room_id:
        print("  🤔 [SKIP] 没有创建房间，跳过删除。")
        return
        
    # 用户B（非创建者）删除房间失败
    response_del_fail = session_b.delete(f"{BASE_URL}/api/v1/rooms/{room_id}")
    if check_response(response_del_fail, 403, expect_success=False):
        print_pass("DELETE /rooms/{id} - 非创建者删除房间被拒绝（预期行为）.")

    # 用户A（创建者）删除房间成功
    response_del_ok = session_a.delete(f"{BASE_URL}/api/v1/rooms/{room_id}")
    if check_response(response_del_ok, 200, ["room_id", "deleted_by"], {"data.deleted_by": test_state['user_a']['id']}):
        print_pass("DELETE /rooms/{id} - 创建者删除房间成功.")

def main():
    """主函数，按顺序执行所有API测试"""
    session_a = requests.Session()
    session_b = requests.Session()
    
    try:
        test_01_system_apis()
        test_02_auth_flow(session_a, session_b)
        test_03_user_management(session_a)
        test_04_room_management(session_a, session_b)
        test_05_message_management(session_a)
    except requests.exceptions.ConnectionError:
        print_fail(f"无法连接到服务器 {BASE_URL}. 请确保您的API服务器正在运行。")
    except Exception as e:
        print_fail(f"测试期间发生未知错误: {e}")
    finally:
        if test_state.get('room'):
             test_99_cleanup(session_a, session_b)
        
        print("\n" + "="*60)
        print("📊  API测试总结")
        print(f"  总计: {sum(test_summary.values())}, 通过: {test_summary['passed']}, 失败: {test_summary['failed']}")
        print("="*60)
        
        if test_summary["failed"] > 0:
            sys.exit(1)

if __name__ == "__main__":
    main()