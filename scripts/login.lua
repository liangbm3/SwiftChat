-- example.lua

-- setup 函数：在每个线程开始时被调用
function setup(thread)
  -- 为每个线程预先定义一个请求体
  -- 这样可以避免在 request 函数中为每次请求都重新创建，提高性能
  request_body = '{"username": "1234", "password": "1234"}'
end

-- request 函数：在每次请求发出前被调用
function request()
  -- 设置请求方法
  wrk.method = "POST"
  
  -- 设置请求路径
  wrk.path = "/api/v1/auth/login"
  
  -- 设置请求头
  wrk.headers["Content-Type"] = "application/json"
  
  -- 设置请求体
  wrk.body = request_body
  
  -- wrk 会自动返回并发出这个请求
  return wrk.request()
end

-- response 函数：在每次收到响应后被调用
function response(status, headers, body)
  -- 简单地检查响应状态码是否为 201 (Created)
  if status ~= 200 then
    print("Unexpected status code: " .. status)
  end
end