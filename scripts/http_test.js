import http from 'k6/http';

import { check, sleep } from 'k6';

import { randomString } from 'https://jslib.k6.io/k6-utils/1.2.0/index.js';



// --- 测试配置 ---

export const options = {

  // 定义虚拟用户(VU)和测试时长

  stages: [

    { duration: '20s', target: 10 },   // 20秒内，从0 ramp-up 到 10个虚拟用户

    { duration: '1m', target: 10 },    // 维持10个虚拟用户运行1分钟

    { duration: '20s', target: 30 },   // 20秒内增加到30个虚拟用户

    { duration: '1m', target: 30 },    // 维持30个虚拟用户运行1分钟

    { duration: '15s', target: 0 },    // 15秒内 ramp-down 到 0

  ],

  // 定义全局阈值，测试不达标时会失败

  thresholds: {

    'http_req_duration': ['p(95)<1000'], // 95%的请求响应时间必须低于1秒

    'http_req_failed': ['rate<0.05'],    // 请求失败率必须低于5%

    'http_req_duration{name:GetRooms}': ['p(95)<500'],     // 获取房间列表响应时间

    'http_req_duration{name:GetMe}': ['p(95)<300'],        // 获取用户信息响应时间

    'http_req_duration{name:CreateRoom}': ['p(95)<800'],   // 创建房间响应时间

    'http_req_duration{name:JoinRoom}': ['p(95)<500'],     // 加入房间响应时间

  },

};



// --- 准备工作 (在测试开始前运行一次) ---

export function setup() {

  console.log('🚀 开始性能测试准备工作...');

 

  // 创建一个基础房间供测试使用

  const baseUser = {

    username: `base_user_${randomString(8)}`,

    password: 'password123'

  };

 

  // 注册基础用户

  const registerRes = http.post(`${__ENV.BASE_URL}/api/v1/auth/register`,

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

    description: 'k6性能测试专用房间'

  };

 

  const roomRes = http.post(`${__ENV.BASE_URL}/api/v1/rooms`,

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

    testRoomId: roomId,

    baseUser: baseUser

  };

}



// --- 虚拟用户执行的脚本 ---

export default function (setupData) {

  const vuId = __VU; // 虚拟用户ID

  const iterationId = __ITER; // 当前迭代ID

 

  // 每个虚拟用户注册自己的账号（除了VU 1使用基础用户）

  let token, userId;

 

  if (vuId === 1) {

    // VU 1 使用基础用户

    token = setupData.baseToken;

    userId = 'base_user';

  } else {

    // 其他VU注册新用户

    const userData = {

      username: `perf_user_${vuId}_${iterationId}_${randomString(4)}`,

      password: 'password123'

    };

   

    const registerRes = http.post(`${__ENV.BASE_URL}/api/v1/auth/register`,

      JSON.stringify(userData),

      {

        headers: { 'Content-Type': 'application/json' },

        tags: { name: 'Register' }

      }

    );

   

    if (!check(registerRes, {

      'Register: status is 201': (r) => r.status === 201,

      'Register: has token': (r) => r.json('data.token') !== undefined,

    })) {

      // 如果注册失败，尝试登录（可能用户已存在）

      const loginRes = http.post(`${__ENV.BASE_URL}/api/v1/auth/login`,

        JSON.stringify(userData),

        {

          headers: { 'Content-Type': 'application/json' },

          tags: { name: 'Login' }

        }

      );

     

      if (loginRes.status === 200) {

        token = loginRes.json('data.token');

        userId = loginRes.json('data.id');

      } else {

        console.error(`VU ${vuId}: 注册和登录都失败`);

        return;

      }

    } else {

      token = registerRes.json('data.token');

      userId = registerRes.json('data.id');

    }

  }



  const headers = {

    'Authorization': `Bearer ${token}`,

    'Content-Type': 'application/json',

  };



  // 模拟真实用户行为场景

  performUserScenario(headers, setupData, vuId);

 

  // 随机等待时间，模拟真实用户行为

  sleep(Math.random() * 2 + 1); // 1-3秒随机等待

}



function performUserScenario(headers, setupData, vuId) {

  // 场景1: 获取房间列表

  const roomsRes = http.get(`${__ENV.BASE_URL}/api/v1/rooms`, {

    headers: headers,

    tags: { name: 'GetRooms' },

  });



  check(roomsRes, {

    'GetRooms: status is 200': (r) => r.status === 200,

    'GetRooms: has rooms data': (r) => r.json('data.rooms') !== undefined,

  });



  // 场景2: 获取自己的用户信息

  const meRes = http.get(`${__ENV.BASE_URL}/api/v1/users/me`, {

    headers: headers,

    tags: { name: 'GetMe' },

  });



  check(meRes, {

    'GetMe: status is 200': (r) => r.status === 200,

    'GetMe: has user data': (r) => r.json('data.user') !== undefined,

  });



  // 场景3: 随机决定是否创建新房间（30%概率）

  if (Math.random() < 0.3) {

    const roomData = {

      name: `Room_VU${vuId}_${randomString(4)}`,

      description: `由VU${vuId}创建的测试房间`

    };

   

    const createRoomRes = http.post(`${__ENV.BASE_URL}/api/v1/rooms`,

      JSON.stringify(roomData),

      {

        headers: headers,

        tags: { name: 'CreateRoom' }

      }

    );

   

    check(createRoomRes, {

      'CreateRoom: status is 201': (r) => r.status === 201,

      'CreateRoom: has room id': (r) => r.json('data.id') !== undefined,

    });

   

    if (createRoomRes.status === 201) {

      const newRoomId = createRoomRes.json('data.id');

     

      // 短暂等待后尝试更新房间描述

      sleep(0.5);

      const updateData = { description: `更新的描述 - ${new Date().toISOString()}` };

     

      const updateRes = http.patch(`${__ENV.BASE_URL}/api/v1/rooms/${newRoomId}`,

        JSON.stringify(updateData),

        {

          headers: headers,

          tags: { name: 'UpdateRoom' }

        }

      );

     

      check(updateRes, {

        'UpdateRoom: status is 200': (r) => r.status === 200,

      });

    }

  }



  // 场景4: 加入现有测试房间（如果存在）

  if (setupData.testRoomId && Math.random() < 0.6) {

    const joinData = { room_id: setupData.testRoomId };

   

    const joinRes = http.post(`${__ENV.BASE_URL}/api/v1/rooms/join`,

      JSON.stringify(joinData),

      {

        headers: headers,

        tags: { name: 'JoinRoom' }

      }

    );

   

    check(joinRes, {

      'JoinRoom: status is 200 or 409': (r) => r.status === 200 || r.status === 409, // 409表示已经是成员

    });

   

    // 如果加入成功，获取已加入的房间列表

    if (joinRes.status === 200) {

      sleep(0.2);

     

      const joinedRoomsRes = http.get(`${__ENV.BASE_URL}/api/v1/rooms/joined`, {

        headers: headers,

        tags: { name: 'GetJoinedRooms' },

      });

     

      check(joinedRoomsRes, {

        'GetJoinedRooms: status is 200': (r) => r.status === 200,

      });

    }

  }



  // 场景5: 获取用户列表（分页测试）

  if (Math.random() < 0.4) {

    const usersRes = http.get(`${__ENV.BASE_URL}/api/v1/users?limit=10&offset=0`, {

      headers: headers,

      tags: { name: 'GetUsers' },

    });

   

    check(usersRes, {

      'GetUsers: status is 200': (r) => r.status === 200,

      'GetUsers: has pagination': (r) => {

        const data = r.json('data');

        return data && data.hasOwnProperty('total') && data.hasOwnProperty('limit');

      },

    });

  }

}



// --- 测试结束后清理工作 ---

export function teardown(setupData) {

  console.log('🧹 开始清理测试数据...');

 

  if (setupData && setupData.testRoomId && setupData.baseToken) {

    // 删除测试房间

    const deleteRes = http.del(`${__ENV.BASE_URL}/api/v1/rooms/${setupData.testRoomId}`, null, {

      headers: {

        'Authorization': `Bearer ${setupData.baseToken}`

      }

    });

   

    if (deleteRes.status === 200) {

      console.log('✅ 测试房间已清理');

    } else {

      console.log(`⚠️  清理测试房间失败: ${deleteRes.status}`);

    }

  }

 

  console.log('📊 性能测试完成！');

}