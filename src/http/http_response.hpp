#pragma once
#include <string>
#include <unordered_map>

namespace http
{
    class HttpResponse
    {
    public:
        int status_code; // 状态码，如 200、404 等
        std::string body;
        std::unordered_map<std::string, std::string> headers; // 响应头

        HttpResponse(int code=200,const std::string &body_content="");
        std::string toString() const; // 将响应转换为字符串格式
    private:
        static std::string getStatusText(int code); // 根据状态码获取状态文本
    };
}