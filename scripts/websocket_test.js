// 导入 k6 模块
import http from 'k6/http';
import ws from 'k6/ws';
import { check, sleep, group } from 'k6';
import { randomString } from 'https://jslib.k6.io/k6-utils/1.2.0/index.js';
import { Trend } from 'k6/metrics';

// --- 自定义指标 ---
// 用于测量从发送消息到收到服务器确认回执的往返时间
const sendMessageRTT = new Trend('ws_send_message_rtt', true);
// 用于测量从发送请求到收到响应的延迟
const authResponseTrend = new Trend('ws_auth_response_time', true);
const joinRoomResponseTrend = new Trend('ws_join_room_response_time', true);


// --- 测试配置 ---
export const options = {
  // 定义虚拟用户(VU)和测试时长 (与HTTP脚本保持一致)
  stages: [
    { duration: '20s', target: 10 },   // 20秒内，从0 ramp-up 到 10个虚拟用户
    { duration: '1m', target: 10 },    // 维持10个虚拟用户运行1分钟
    { duration: '20s', target: 30 },   // 20秒内增加到30个虚拟用户
    { duration: '1m', target: 30 },    // 维持30个虚拟用户运行1分钟
    { duration: '15s', target: 0 },    // 15秒内 ramp-down 到 0
  ],
  // 定义全局阈值
  thresholds: {
    'http_req_failed': ['rate<0.05'],      // HTTP注册/登录请求失败率
    'ws_connecting': ['p(95)<1000'],       // 95%的WebSocket连接建立时间应低于1秒
    'ws_session_duration': ['p(95)<120000'], // 95%的会话时长应低于2分钟
    'ws_send_message_rtt': ['p(95)<500'],  // 95%的消息发送->确认 RTT 应低于500ms
    'ws_auth_response_time': ['p(95)<300'],// 95%的WS认证响应时间应低于300ms
    'ws_join_room_response_time': ['p(95)<400'],// 95%的加入房间响应时间应低于400ms
  },
};

// --- 准备工作 (在测试开始前运行一次) ---
// 这部分与您的HTTP脚本完全相同，因为我们需要先通过HTTP API创建测试环境
export function setup() {
  console.log('🚀 开始WebSocket性能测试准备工作 (通过HTTP API)...');
  
  // 确保 BASE_URL 环境变量已设置
  if (!__ENV.BASE_URL) {
    throw new Error('请设置环境变量 BASE_URL (例如: k6 run -e BASE_URL=localhost:8080 script.js)');
  }
  
  const baseUser = {
    username: `base_user_${randomString(8)}`,
    password: 'password123'
  };
  
  // 注册基础用户
  const registerRes = http.post(`http://${__ENV.BASE_URL}/api/v1/auth/register`, 
    JSON.stringify(baseUser),
    { headers: { 'Content-Type': 'application/json' } }
  );
  
  if (registerRes.status !== 201) {
    throw new Error(`无法注册基础用户: ${registerRes.status} - ${registerRes.body}`);
  }
  
  const baseToken = registerRes.json('data.token');
  
  // 创建一个测试房间
  const roomData = {
    name: `PerfTest_Room_${randomString(6)}`,
    description: 'k6 WebSocket性能测试专用房间'
  };
  
  const roomRes = http.post(`http://${__ENV.BASE_URL}/api/v1/rooms`,
    JSON.stringify(roomData),
    { headers: { 
      'Content-Type': 'application/json',
      'Authorization': `Bearer ${baseToken}`
    }}
  );
  
  if (roomRes.status !== 201) {
    throw new Error(`无法创建测试房间: ${roomRes.status} - ${roomRes.body}`);
  }
  
  const roomId = roomRes.json('data.id');
  console.log(`✅ 准备工作完成，创建了测试房间: ${roomId}`);
  
  return {
    baseToken: baseToken,
    testRoomId: roomId
  };
}

// --- 虚拟用户执行的脚本 ---
export default function (setupData) {
  const vuId = __VU;
  const iterationId = __ITER;
  
  // 1. 获取认证 Token (通过HTTP)
  // 每个VU注册自己的账号以获得独立的token
  const userData = {
    username: `ws_user_${vuId}_${iterationId}_${randomString(4)}`,
    password: 'password123'
  };
  
  const registerRes = http.post(`http://${__ENV.BASE_URL}/api/v1/auth/register`,
      JSON.stringify(userData),
      { headers: { 'Content-Type': 'application/json' }, tags: { name: 'HTTP-Register' } }
  );

  let token;
  if (registerRes.status === 201) {
      token = registerRes.json('data.token');
  } else {
      // 如果注册失败（可能用户已存在），尝试登录
      const loginRes = http.post(`http://${__ENV.BASE_URL}/api/v1/auth/login`,
          JSON.stringify(userData),
          { headers: { 'Content-Type': 'application/json' }, tags: { name: 'HTTP-Login' } }
      );
      if (loginRes.status === 200) {
          token = loginRes.json('data.token');
      } else {
          console.error(`VU ${vuId}: 注册和登录都失败了, 无法进行WebSocket测试.`);
          return; // 无法获取token，此VU无法继续测试
      }
  }

  // 2. 连接 WebSocket
  // 确保 WS_URL 环境变量已设置
  if (!__ENV.WS_URL) {
    throw new Error('请设置环境变量 WS_URL (例如: k6 run -e WS_URL=localhost:8081 ... script.js)');
  }
  const wsUrl = `ws://${__ENV.WS_URL}`;
  
  const res = ws.connect(wsUrl, null, function (socket) {
    // A. 连接建立后的回调函数
    socket.on('open', function open() {
      console.log(`VU ${vuId}: WebSocket 连接已建立.`);
      
      // B. 发送认证消息
      const authPayload = { type: 'auth', token: token };
      const authStartTime = new Date();
      socket.send(JSON.stringify(authPayload));
      
      // 设置定时心跳 (ping)
      socket.setInterval(function timeout() {
        socket.send(JSON.stringify({ type: 'ping' }));
        // console.log(`VU ${vuId}: 发送 ping`);
      }, 25000); // 每25秒发送一次心跳
    });

    // C. 监听服务器消息
    socket.on('message', function (data) {
      const msg = JSON.parse(data);
      
      // 检查收到的每条消息是否成功
      check(msg, { 'WebSocket message.success is true': (m) => m.success === true });

      if (msg.data && msg.data.type) {
        switch (msg.data.type) {
          case 'connected':
            // 认证成功
            authResponseTrend.add(new Date() - authStartTime);
            check(msg, {'WS Auth: achnowledged': (m) => m.message === 'WebSocket authentication successful'});
            console.log(`VU ${vuId}: WebSocket 认证成功.`);
            
            // D. 加入房间
            const joinPayload = { type: 'join_room', room_id: setupData.testRoomId };
            const joinStartTime = new Date();
            socket.send(JSON.stringify(joinPayload));
            break;
            
          case 'room_joined':
            // 加入房间成功
            joinRoomResponseTrend.add(new Date() - joinStartTime);
            check(msg, {'WS Join: acknowledged': (m) => m.message === 'Room joined successfully'});
            console.log(`VU ${vuId}: 成功加入房间 ${msg.data.room_id}.`);
            
            // E. 开始模拟发送聊天消息
            simulateChatting(socket, vuId);
            break;
            
          case 'message_sent':
            // 自己发送的消息得到服务器确认
            sendMessageRTT.add(new Date() - socket.messageSendTime);
            check(msg, {'WS SendMsg: acknowledged': (m) => m.message === 'Message sent successfully'});
            break;

          case 'message_received':
            // 收到别人的消息（广播）
            // console.log(`VU ${vuId}: 收到来自 ${msg.data.username} 的消息: ${msg.data.content}`);
            break;

          case 'user_joined':
             // console.log(`VU ${vuId}: 用户 ${msg.data.username} 加入了房间.`);
            break;
            
          case 'user_left':
             // console.log(`VU ${vuId}: 用户 ${msg.data.username} 离开了房间.`);
            break;
            
          case 'pong':
            // 收到心跳响应
            // console.log(`VU ${vuId}: 收到 pong`);
            break;
            
          default:
            // console.log(`VU ${vuId}: 收到未知类型的消息: ${msg.data.type}`);
        }
      }
    });

    // F. 连接关闭事件
    socket.on('close', function () {
      console.log(`VU ${vuId}: WebSocket 连接已关闭.`);
    });
    
    // G. 错误处理
    socket.on('error', function(e) {
      if (e.error() != 'websocket: close sent') {
        console.error(`VU ${vuId}: WebSocket 发生错误: ${e.error()}`);
      }
    });

    // H. 等待一段时间后主动关闭连接，以结束此VU的迭代
    socket.setTimeout(function () {
      console.log(`VU ${vuId}: 会话超时，准备关闭连接.`);
      socket.close();
    }, 60000 + Math.random() * 10000); // 每个会话持续60-70秒
  });

  check(res, { 'WebSocket 连接成功': (r) => r && r.status === 101 });
}

// 模拟聊天行为的函数
function simulateChatting(socket, vuId) {
  // 随机发送3到8条消息
  const messageCount = Math.floor(Math.random() * 6) + 3;
  for (let i = 0; i < messageCount; i++) {
    // 模拟用户输入和思考的间隔
    sleep(Math.random() * 5 + 2); // 2-7秒
    
    const messagePayload = {
      type: 'send_message',
      content: `来自 VU ${vuId} 的第 ${i + 1} 条性能测试消息: ${randomString(10)}`
    };
    
    // 记录发送时间点，用于计算RTT
    socket.messageSendTime = new Date(); 
    socket.send(JSON.stringify(messagePayload));
  }
}


// --- 测试结束后清理工作 ---
export function teardown(setupData) {
  console.log('🧹 开始清理测试数据...');
  
  if (setupData && setupData.testRoomId && setupData.baseToken) {
    // 使用基础用户token删除测试房间
    const deleteRes = http.del(`http://${__ENV.BASE_URL}/api/v1/rooms/${setupData.testRoomId}`, null, {
      headers: { 'Authorization': `Bearer ${setupData.baseToken}` }
    });
    
    if (deleteRes.status === 200) {
      console.log('✅ 测试房间已成功清理.');
    } else {
      console.error(`⚠️ 清理测试房间失败: ${deleteRes.status} - ${deleteRes.body}`);
    }
  }
  
  console.log('📊 WebSocket 性能测试完成！');
}