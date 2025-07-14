#include "http_response.hpp"
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <cstdio>

namespace http
{
    // 辅助函数：转义JSON字符串中的特殊字符
    std::string escapeJsonString(const std::string& str) {
        std::string escaped;
        escaped.reserve(str.length() * 2); // 预留空间
        
        for (char c : str) {
            switch (c) {
                case '"': escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\b': escaped += "\\b"; break;
                case '\f': escaped += "\\f"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default:
                    if (c < 0x20) {
                        // 控制字符用unicode转义
                        std::stringstream uss;
                        uss << "\\u" << std::hex << std::setw(4) << std::setfill('0') 
                            << static_cast<unsigned char>(c);
                        escaped += uss.str();
                    } else {
                        escaped += c;
                    }
                    break;
            }
        }
        return escaped;
    }
    
    // 辅助函数：简单检查是否为JSON格式
    bool isJsonLike(const std::string& str) {
        if (str.empty()) return false;
        
        // 去除首尾空白字符
        auto start = str.find_first_not_of(" \t\n\r");
        auto end = str.find_last_not_of(" \t\n\r");
        
        if (start == std::string::npos) return false;
        
        char first = str[start];
        char last = str[end];
        
        return (first == '{' && last == '}') || (first == '[' && last == ']');
    }
    
    // 辅助函数：获取当前时间的HTTP格式字符串
    std::string getHttpDate() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t), "%a, %d %b %Y %H:%M:%S GMT");
        return ss.str();
    }

    HttpResponse::HttpResponse(int code, const std::string &body_content)
        : status_code(code), body(body_content)
    {
        // 设置默认响应头
        headers["Server"] = "SwiftChat/1.0";
        headers["Date"] = getHttpDate();
        headers["Connection"] = "close";
        
        // 处理响应体和Content-Type
        if (body.empty()) {
            headers["Content-Type"] = "text/plain";
        } else if (isJsonLike(body)) {
            headers["Content-Type"] = "application/json; charset=utf-8";
        } else {
            // 将普通文本消息转换为JSON格式
            body = "{\"message\":\"" + escapeJsonString(body) + "\"}";
            headers["Content-Type"] = "application/json; charset=utf-8";
        }
    }

    std::string HttpResponse::toString() const
    {
        std::stringstream ss;
        ss << "HTTP/1.1 " << status_code << " " << getStatusText(status_code) << "\r\n";
        ss << "Content-Length: " << body.length() << "\r\n";
        for (const auto &header : headers)
        {
            ss << header.first << ": " << header.second << "\r\n";
        }
        ss << "\r\n";
        ss << body;
        return ss.str();
    }

    std::string HttpResponse::getStatusText(int code)
    {
        switch (code)
        {
        case 200:
            return "OK";
        case 201:
            return "Created";
        case 302:
            return "Found";
        case 400:
            return "Bad Request";
        case 401:
            return "Unauthorized";
        case 403:
            return "Forbidden";
        case 404:
            return "Not Found";
        case 409:
            return "Conflict";
        case 500:
            return "Internal Server Error";
        default:
            return "Unknown";
        }
    }
}