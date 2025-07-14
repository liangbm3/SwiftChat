#include "http_request.hpp"
#include "utils/logger.hpp"
#include <sstream>
#include <vector>
#include <iostream>
#include <cctype>

namespace http
{
    // 辅助函数：去除字符串末尾的回车符
    std::string trimCarriageReturn(const std::string& str) {
        if (!str.empty() && str.back() == '\r') {
            return str.substr(0, str.length() - 1);
        }
        return str;
    }

    // 解析原始的HTTP请求字符串，生成一个HTTPRequest对象
    HttpRequest HttpRequest::parse(const std::string &raw_request)
    {
        HttpRequest request;//创建一个空的HttpRequest对象
        std::istringstream iss(raw_request);//包装为输入流，方便逐行解析
        std::string line;

        // 解析请求行
        if (!std::getline(iss, line)) {
            LOG_ERROR << "Failed to read request line";
            return request; // 返回空的request对象
        }
        
        line = trimCarriageReturn(line);
        std::istringstream request_line(line);
        request_line >> request.method; // 读取请求方法
        request_line >> request.path;   // 读取请求路径
        request_line >> request.version; // 读取HTTP版本

        //解析查询参数
        request.parseQueryParams();
        
        // 解析请求头
        while (std::getline(iss, line)) {
            line = trimCarriageReturn(line);
            
            // 空行表示头部结束
            if (line.empty()) {
                break;
            }
            
            auto colon_pos = line.find(':'); // 查找冒号位置
            if (colon_pos != std::string::npos) {
                std::string key = line.substr(0, colon_pos); // 获取键
                std::string value = line.substr(colon_pos + 1); // 获取值
                
                // 去除值开头的空格
                while (!value.empty() && value[0] == ' ') {
                    value = value.substr(1);
                }
                
                request.headers[key] = value; // 存储到请求头映射中
            }
        }

        // 如果 Content-Length 头存在，读取请求体
        auto content_length_it = request.headers.find("Content-Length");
        if (content_length_it != request.headers.end()) {
            try {
                size_t content_length = std::stoul(content_length_it->second);
                
                if (content_length > 0) {
                    // 读取请求体
                    std::vector<char> body_buffer(content_length);
                    iss.read(body_buffer.data(), content_length);
                    size_t bytes_read = iss.gcount();
                    
                    request.body = std::string(body_buffer.data(), bytes_read);
                    
                    LOG_DEBUG << "Read body with length " << bytes_read
                              << " (expected " << content_length << ")";
                }
            } catch (const std::exception& e) {
                LOG_ERROR << "Invalid Content-Length value: " << content_length_it->second;
            }
        }
        
        return request; // 返回解析后的请求对象
    }

    void HttpRequest::parseQueryParams()
    {
        auto pos = path.find('?');// 如果路径中包含查询参数，进行解析
        if(pos != std::string::npos)
        {
            std::string params_str = path.substr(pos + 1); // 获取查询参数部分
            path = path.substr(0, pos); // 更新路径，去掉查询参数
            size_t start = 0;
            size_t end;
            while ((end = params_str.find('&', start)) != std::string::npos)
            {
                parseParams(params_str.substr(start, end - start)); // 解析单个查询参数
                start = end + 1; // 更新起始位置
            }
            if (start < params_str.length()) // 处理最后一个参数
            {
                parseParams(params_str.substr(start));
            }
        }
    }

    void HttpRequest::parseParams(const std::string &params_str)
    {
        size_t equals_pos = params_str.find('=');// 查找等号位置
        if (equals_pos != std::string::npos)
        {
            std::string key = urlDecode(params_str.substr(0, equals_pos)); // 获取键
            std::string value = urlDecode(params_str.substr(equals_pos + 1)); // 获取值
            query_params[key] = value; // 存储到查询参数映射中
        }
    }

    std::string HttpRequest::urlDecode(const std::string &encoded_str)
    {
        std::string decoded_str;
        for (size_t i = 0; i < encoded_str.length(); i++)
        {
            // 如果当前字符是百分号，则表示后面跟着两个十六进制数字
            if (encoded_str[i] == '%' && i + 2 < encoded_str.length())
            {
                // 检查后面两个字符是否为有效的十六进制数字
                if (std::isxdigit(encoded_str[i + 1]) && std::isxdigit(encoded_str[i + 2])) {
                    int value;
                    // 将后面的两个十六进制数字转换为字符
                    if (sscanf(encoded_str.substr(i + 1, 2).c_str(), "%x", &value) == 1) {
                        decoded_str += static_cast<char>(value);
                        i += 2; // 跳过两个十六进制字符
                    } else {
                        decoded_str += encoded_str[i]; // 如果转换失败，保留原字符
                    }
                } else {
                    decoded_str += encoded_str[i]; // 无效的十六进制，保留原字符
                }
            }
            else if (encoded_str[i] == '+')
            {
                decoded_str += ' '; // 将 '+' 替换为空格
            }
            else
            {
                decoded_str += encoded_str[i]; // 直接添加其他字符
            }
        }
        return decoded_str;
    }
}