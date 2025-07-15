#include "http_response.hpp"
#include <sstream>
#include <chrono>
#include <iomanip>

namespace http
{

    HttpResponse::HttpResponse() : status_code_(200)
    {
        // 设置最基本的默认响应头
        headers_["Server"] = "SwiftChat/1.0";
        headers_["Date"] = getHttpDate();
        headers_["Connection"] = "close";
    }

    // --- 流式接口实现 ---

    HttpResponse &HttpResponse::withStatus(int code)
    {
        status_code_ = code;
        return *this;
    }

    HttpResponse &HttpResponse::withHeader(const std::string &key, const std::string &value)
    {
        headers_[key] = value;
        return *this;
    }

    HttpResponse &HttpResponse::withBody(const std::string &body_content, const std::string &content_type)
    {
        body_ = body_content;
        headers_["Content-Type"] = content_type;
        return *this;
    }

    HttpResponse &HttpResponse::withJsonBody(const nlohmann::json &json_body)
    {
        body_ = json_body.dump(); // 使用库进行序列化
        headers_["Content-Type"] = "application/json; charset=utf-8";
        return *this;
    }

    // --- 静态工厂方法实现 ---
    HttpResponse HttpResponse::Ok(const std::string &body)
    {
        return HttpResponse().withStatus(200).withBody(body);
    }

    HttpResponse HttpResponse::Created(const std::string &body)
    {
        return HttpResponse().withStatus(201).withJsonBody({{"message", body}});
    }

    HttpResponse HttpResponse::BadRequest(const std::string &error_message)
    {
        return HttpResponse().withStatus(400).withJsonBody({{"error", error_message}});
    }

    HttpResponse HttpResponse::Unauthorized(const std::string &error_message)
    {
        return HttpResponse().withStatus(401).withJsonBody({{"error", error_message}});
    }

    HttpResponse HttpResponse::Forbidden(const std::string &error_message)
    {
        return HttpResponse().withStatus(403).withJsonBody({{"error", error_message}});
    }

    HttpResponse HttpResponse::NotFound(const std::string &error_message)
    {
        return HttpResponse().withStatus(404).withJsonBody({{"error", error_message}});
    }

    HttpResponse HttpResponse::InternalError(const std::string &error_message)
    {
        return HttpResponse().withStatus(500).withJsonBody({{"error", error_message}});
    }

    HttpResponse HttpResponse::NoContent()
    {
        // 创建一个204响应。根据规范，body应为空。
        return HttpResponse().withStatus(204).withBody("");
    }

    // --- 序列化 ---
    std::string HttpResponse::toString() const
    {
        std::stringstream ss;
        ss << "HTTP/1.1 " << status_code_ << " " << getStatusText(status_code_) << "\r\n";

        // 确保Content-Length总是最新的
        ss << "Content-Length: " << body_.length() << "\r\n";

        for (const auto &header : headers_)
        {
            ss << header.first << ": " << header.second << "\r\n";
        }
        ss << "\r\n";
        ss << body_;
        return ss.str();
    }

    // --- 私有辅助函数 ---
    std::string HttpResponse::getHttpDate()
    {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t), "%a, %d %b %Y %H:%M:%S GMT");
        return ss.str();
    }

    std::string HttpResponse::getStatusText(int code)
    {
        // (这里的 switch 语句实现与您原来的一样，保持不变)
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