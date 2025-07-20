#include "http_request.hpp"
#include "utils/logger.hpp"
#include <sstream>
#include <vector>
#include <stdexcept>

namespace http
{
    // 静态工厂方法，用于解析请求并创建对象
    // 在 http_request.cpp 中

    std::optional<HttpRequest> HttpRequest::parse(const std::string &raw_request)
    {
        // 1. 查找头部和身体的分隔符 "\r\n\r\n"
        size_t head_end_pos = raw_request.find("\r\n\r\n");
        if (head_end_pos == std::string::npos)
        {
            LOG_ERROR << "Malformed request: Missing header/body separator (\\r\\n\\r\\n).";
            return std::nullopt;
        }

        std::string head_str = raw_request.substr(0, head_end_pos);
        std::istringstream head_ss(head_str);

        HttpRequest request;
        std::string line;

        // 2. 解析请求行 (Method Path Version)
        if (!std::getline(head_ss, line) || line.empty())
        {
            LOG_ERROR << "Failed to read request line or request is empty.";
            return std::nullopt;
        }
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        std::istringstream request_line_ss(line);
        if (!(request_line_ss >> request.method_ >> request.path_ >> request.version_))
        {
            LOG_ERROR << "Malformed request line: " << line;
            return std::nullopt;
        }

        // 从完整路径中分离出查询参数
        auto query_pos = request.path_.find('?');
        if (query_pos != std::string::npos)
        {
            request.parseQueryParams(request.path_.substr(query_pos + 1));
            request.path_ = request.path_.substr(0, query_pos);
        }

        // 3. 解析请求头
        while (std::getline(head_ss, line))
        {
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }
            if (line.empty())
                continue; // 忽略头部中的空行

            auto colon_pos = line.find(':');
            if (colon_pos != std::string::npos)
            {
                std::string key = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                request.headers_[key] = value;
            }
        }

        // 4. 解析 Cookies
        if (request.hasHeader("Cookie"))
        {
            request.parseCookies(request.getHeaderValue("Cookie").value().data());
        }

        // 5. 解析请求体
        if (request.hasHeader("Content-Length"))
        {
            try
            {
                size_t content_length = std::stoul(request.getHeaderValue("Content-Length").value().data());
                size_t body_start_pos = head_end_pos + 4; // "\r\n\r\n" 是4个字符

                if (raw_request.length() < body_start_pos + content_length)
                {
                    LOG_ERROR << "Incomplete request body. Expected " << content_length
                              << " bytes, but only " << (raw_request.length() - body_start_pos) << " available.";
                    return std::nullopt;
                }
                request.body_ = raw_request.substr(body_start_pos, content_length);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Invalid Content-Length value: " << e.what();
                return std::nullopt;
            }
        }

        return request;
    }

    // --- Getters and Helpers ---

    bool HttpRequest::hasHeader(const std::string &key) const
    {
        return headers_.count(key) > 0;
    }

    std::optional<std::string_view> HttpRequest::getHeaderValue(const std::string &key) const
    {
        auto it = headers_.find(key);
        if (it != headers_.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

    bool HttpRequest::hasQueryParam(const std::string &key) const
    {
        return query_params_.count(key) > 0;
    }

    std::optional<std::string_view> HttpRequest::getQueryParam(const std::string &key) const
    {
        auto it = query_params_.find(key);
        if (it != query_params_.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

    bool HttpRequest::hasCookie(const std::string &key) const
    {
        return cookies_.count(key) > 0;
    }

    std::optional<std::string_view> HttpRequest::getCookieValue(const std::string &key) const
    {
        auto it = cookies_.find(key);
        if (it != cookies_.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

    // --- Private Methods ---

    void HttpRequest::parseQueryParams(const std::string &query_str)
    {
        std::istringstream iss(query_str);
        std::string pair;
        while (std::getline(iss, pair, '&'))
        {
            auto eq_pos = pair.find('=');
            if (eq_pos != std::string::npos)
            {
                std::string key = urlDecode(pair.substr(0, eq_pos));
                std::string value = urlDecode(pair.substr(eq_pos + 1));
                if (!key.empty())
                {
                    query_params_[key] = value;
                }
            }
        }
    }

    void HttpRequest::parseCookies(const std::string &cookie_str)
    {
        std::istringstream iss(cookie_str);
        std::string pair;
        while (std::getline(iss, pair, ';'))
        {
            // 去除前导空格
            pair.erase(0, pair.find_first_not_of(" \t"));
            auto eq_pos = pair.find('=');
            if (eq_pos != std::string::npos)
            {
                std::string key = pair.substr(0, eq_pos);
                std::string value = pair.substr(eq_pos + 1);
                if (!key.empty())
                {
                    cookies_[key] = value;
                }
            }
        }
    }

    std::string HttpRequest::urlDecode(const std::string &encoded_str)
    {
        std::string decoded_str;
        decoded_str.reserve(encoded_str.length());
        for (size_t i = 0; i < encoded_str.length(); ++i)
        {
            if (encoded_str[i] == '%' && i + 2 < encoded_str.length())
            {
                if (std::isxdigit(encoded_str[i + 1]) && std::isxdigit(encoded_str[i + 2]))
                {
                    try
                    {
                        std::string hex_str = encoded_str.substr(i + 1, 2);
                        int value = std::stoi(hex_str, nullptr, 16);
                        decoded_str += static_cast<char>(value);
                        i += 2;
                    }
                    catch (const std::exception &e)
                    {
                        decoded_str += encoded_str[i]; // 转换失败则保留原样
                    }
                }
                else
                {
                    decoded_str += encoded_str[i]; // 无效的十六进制，保留原样
                }
            }
            else if (encoded_str[i] == '+')
            {
                decoded_str += ' ';
            }
            else
            {
                decoded_str += encoded_str[i];
            }
        }
        return decoded_str;
    }

    // 路径参数相关方法实现
    bool HttpRequest::hasPathParam(const std::string& key) const
    {
        return path_params_.find(key) != path_params_.end();
    }

    std::optional<std::string_view> HttpRequest::getPathParam(const std::string& key) const
    {
        auto it = path_params_.find(key);
        if (it != path_params_.end())
        {
            return std::string_view(it->second);
        }
        return std::nullopt;
    }
}