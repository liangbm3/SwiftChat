#pragma once
#include <string>
#include <unordered_map>

namespace http
{
    class HttpRequest
    {
    public:
        std::string method; // 请求方法，如 GET、POST 等
        std::string path;   // 请求路径
        std::string version; // HTTP 版本，如 HTTP/1.1
        std::string body; // 请求体内容
        std::unordered_map<std::string, std::string> headers; // 请求头
        std::unordered_map<std::string, std::string> query_params; // 查询参数

        HttpRequest() = default;

        static HttpRequest parse(const std::string &raw_request);

        // 解析url中的查询参数
        void parseQueryParams();
    
    private:
        void parseParams(const std::string &params_str);//解析单个查询参数
        std::string urlDecode(const std::string &encoded_str);

    };
}