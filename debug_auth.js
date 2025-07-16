#!/usr/bin/env node

const http = require('http');

// 配置
const SERVER_HOST = 'localhost';
const SERVER_PORT = 8080;

// 工具函数：发送HTTP请求
function makeRequest(options, data) {
    return new Promise((resolve, reject) => {
        const req = http.request(options, (res) => {
            let body = '';
            res.on('data', (chunk) => {
                body += chunk;
            });
            res.on('end', () => {
                try {
                    const jsonBody = body ? JSON.parse(body) : {};
                    resolve({
                        statusCode: res.statusCode,
                        headers: res.headers,
                        body: jsonBody
                    });
                } catch (e) {
                    resolve({
                        statusCode: res.statusCode,
                        headers: res.headers,
                        body: body
                    });
                }
            });
        });

        req.on('error', (err) => {
            reject(err);
        });

        if (data) {
            req.write(data);
        }
        req.end();
    });
}

// 测试用户注册
async function testRegister() {
    console.log('1. 测试用户注册...');
    
    const userData = {
        username: 'testuser' + Date.now(),
        password: 'password123'
    };
    
    const options = {
        hostname: SERVER_HOST,
        port: SERVER_PORT,
        path: '/api/v1/auth/register',
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
            'Content-Length': Buffer.byteLength(JSON.stringify(userData))
        }
    };

    try {
        const response = await makeRequest(options, JSON.stringify(userData));
        console.log('   注册响应状态:', response.statusCode);
        console.log('   注册响应内容:', response.body);
        
        if (response.statusCode === 200) {
            return { response, username: userData.username, password: userData.password };
        }
        return null;
    } catch (error) {
        console.error('   注册错误:', error);
        return null;
    }
}

// 测试用户登录
async function testLogin(username, password) {
    console.log('2. 测试用户登录...');
    
    const userData = { username, password };
    
    const options = {
        hostname: SERVER_HOST,
        port: SERVER_PORT,
        path: '/api/v1/auth/login',
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
            'Content-Length': Buffer.byteLength(JSON.stringify(userData))
        }
    };

    try {
        const response = await makeRequest(options, JSON.stringify(userData));
        console.log('   登录响应状态:', response.statusCode);
        console.log('   登录响应内容:', response.body);
        
        if (response.body && response.body.data && response.body.data.token) {
            console.log('   获取到token:', response.body.data.token.substring(0, 30) + '...');
            return response.body.data.token;
        }
        return null;
    } catch (error) {
        console.error('   登录错误:', error);
        return null;
    }
}

// 测试创建房间
async function testCreateRoom(token) {
    console.log('3. 测试创建房间...');
    
    const roomData = {
        name: 'Test Room ' + Date.now()
    };
    
    const options = {
        hostname: SERVER_HOST,
        port: SERVER_PORT,
        path: '/api/v1/rooms',
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
            'Authorization': `Bearer ${token}`,
            'Content-Length': Buffer.byteLength(JSON.stringify(roomData))
        }
    };

    console.log('   发送请求头:');
    console.log('     Content-Type:', options.headers['Content-Type']);
    console.log('     Authorization:', options.headers['Authorization'].substring(0, 30) + '...');

    try {
        const response = await makeRequest(options, JSON.stringify(roomData));
        console.log('   创建房间响应状态:', response.statusCode);
        console.log('   创建房间响应内容:', response.body);
        return response;
    } catch (error) {
        console.error('   创建房间错误:', error);
        return null;
    }
}

// 主测试函数
async function runTests() {
    console.log('开始调试认证流程...\n');

    try {
        // 1. 注册用户
        const registerResult = await testRegister();
        if (!registerResult || registerResult.response.statusCode !== 200) {
            console.error('用户注册失败，无法继续测试');
            return;
        }

        // 提取用户名和密码
        const { username, password } = registerResult;

        console.log('\n---\n');

        // 2. 登录用户
        const token = await testLogin(username, password);
        if (!token) {
            console.error('用户登录失败，无法继续测试');
            return;
        }

        console.log('\n---\n');

        // 3. 创建房间
        const createRoomResponse = await testCreateRoom(token);
        
        console.log('\n---\n');
        console.log('测试完成！');
        
    } catch (error) {
        console.error('测试过程中发生错误:', error);
    }
}

// 运行测试
runTests();
