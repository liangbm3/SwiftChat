#pragma once
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace http
{
    class HttpResponse
    {
    public:
        HttpResponse(); // 默认构造一个 200 OK 的响应

        // --- 流式接口 ---
        HttpResponse &withStatus(int code);
        HttpResponse &withHeader(const std::string &key, const std::string &value);
        HttpResponse &withBody(const std::string &body_content, const std::string &content_type = "text/plain");
        HttpResponse &withJsonBody(const nlohmann::json &json_body);

        // --- 静态工厂方法 ---
        static HttpResponse Ok(const std::string &body = "OK");
        static HttpResponse Created(const std::string &body = "Created");
        static HttpResponse BadRequest(const std::string &error_message = "Bad Request");
        static HttpResponse Unauthorized(const std::string &error_message = "Unauthorized");
        static HttpResponse Forbidden(const std::string &error_message = "Forbidden");
        static HttpResponse NotFound(const std::string &error_message = "Not Found");
        static HttpResponse InternalError(const std::string &error_message = "Internal Server Error");
        static HttpResponse NoContent();

        // 将响应对象序列化为发送给客户端的字符串
        std::string toString() const;

    private:
        int status_code_;
        std::string body_;
        std::unordered_map<std::string, std::string> headers_;

        // 私有辅助函数
        static std::string getStatusText(int code);
        static std::string getHttpDate();
    };
}