#pragma once
#include <string>
#include <unordered_map>
#include <algorithm>
#include <optional>

namespace http
{
    // 自定义哈希函数，用于忽略键的大小写
    struct CaseInsensitiveHasher
    {
        std::size_t operator()(const std::string &key) const
        {
            std::string lower_key;
            lower_key.reserve(key.size());
            std::transform(key.begin(), key.end(), lower_key.begin(), ::tolower); // 将键转换为小写
            return std::hash<std::string>()(lower_key);                           // 使用标准哈希函数
        }
    };

    struct CaseInsensitiveEqual
    {
        bool operator()(const std::string &a, const std::string &b) const
        {
            return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(),
                                                      [](char a, char b)
                                                      { return std::tolower(a) == std::tolower(b); }); // 比较时忽略大小写
        }
    };

    class HttpRequest
    {
    public:
        static std::optional<HttpRequest> parse(const std::string &raw_request); // 成功返回一个HttpRequest对象，失败返回std::nullopt

        //getter方法
        const std::string &getMethod() const { return method_; }
        const std::string &getPath() const { return path_; }
        const std::string &getVersion() const { return version_; }
        const std::string &getBody() const { return body_; }

        //辅助函数
        bool hasHeader(const std::string& key) const;// 检查是否有指定的请求头
        std::optional<std::string_view> getHeaderValue(const std::string& key) const;// 获取指定请求头的值
        
        bool hasQueryParam(const std::string& key) const;// 检查是否有指定的查询参数
        std::optional<std::string_view> getQueryParam(const std::string& key) const;// 获取指定查询参数的值

        bool hasCookie(const std::string& key) const;// 检查是否有指定的Cookie
        std::optional<std::string_view> getCookieValue(const std::string& key) const;// 获取指定Cookie的值

        const std::unordered_map<std::string, std::string>& getQueryParams() const { return query_params_; }

    private:
        HttpRequest() = default; // 构造函数私有化，强制使用静态的parse方法创建对象
        void parseQueryParams(const std::string& query_str); //解析url查询字符串
        void parseCookies(const std::string &cookie_str); //解析Cookie字符串
        std::string urlDecode(const std::string &encoded_str);//对url编码的字符串进行解码
        std::string method_;                                        // 请求方法，如 GET、POST 等
        std::string path_;                                          // 请求路径
        std::string version_;                                       // HTTP 版本，如 HTTP/1.1
        std::string body_;                                          // 请求体内容
        std::unordered_map<std::string, std::string, CaseInsensitiveHasher, CaseInsensitiveEqual> headers_;// 请求头映射，忽略大小写
        std::unordered_map<std::string, std::string> query_params_;// 查询参数映射
        std::unordered_map<std::string, std::string> cookies_;//cookie映射
    };
}