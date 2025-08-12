#include "router.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include "utils/logger.hpp"

namespace http
{
    // 初始化静态MIME类型映射表
    const std::unordered_map<std::string, std::string> Router::MIME_TYPES = {
        {"html", "text/html"},
        {"htm", "text/html"},
        {"css", "text/css"},
        {"js", "application/javascript"},
        {"json", "application/json"},
        {"xml", "application/xml"},
        {"png", "image/png"},
        {"jpg", "image/jpeg"},
        {"jpeg", "image/jpeg"},
        {"gif", "image/gif"},
        {"bmp", "image/bmp"},
        {"svg", "image/svg+xml"},
        {"ico", "image/x-icon"},
        {"txt", "text/plain"},
        {"md", "text/markdown"},
        {"pdf", "application/pdf"},
        {"zip", "application/zip"},
        {"woff", "font/woff"},
        {"woff2", "font/woff2"},
        {"ttf", "font/ttf"},
        {"eot", "application/vnd.ms-fontobject"}
    };

    // 构造函数
    Router::Router() : static_dir_("./static") {}

    void Router::addHandler(const Route &route)
    {
        routes_.push_back(route);
        // 按路径复杂度排序：精确匹配的路由应该排在参数化路由之前
        std::sort(routes_.begin(), routes_.end(), [](const Route &a, const Route &b) {
            // 计算路径中参数的数量
            auto countParams = [](const std::string &path) {
                return std::count(path.begin(), path.end(), '{');
            };
            
            int a_params = countParams(a.path);
            int b_params = countParams(b.path);
            
            // 参数少的路由优先（精确匹配）
            if (a_params != b_params) {
                return a_params < b_params;
            }
            
            // 参数数量相同时，按路径长度排序（更具体的路径优先）
            return a.path.length() > b.path.length();
        });
    }

    void Router::setMiddleware(Middleware middleware)
    {
        this->middleware_ = std::move(middleware); // 移动语义
    }

    void Router::setStaticDirectory(const std::string &dir)
    {
        static_dir_ = dir;
    }

    HttpResponse Router::route(const HttpRequest &request)
    {
        // 处理所有 OPTIONS 请求（CORS 预检）
        if (request.getMethod() == "OPTIONS")
        {
            LOG_INFO << "Handling CORS preflight request for: " << request.getPath();
            return HttpResponse::Ok()
                .withHeader("Access-Control-Allow-Origin", "*")
                .withHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
                .withHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With")
                .withHeader("Access-Control-Max-Age", "86400") // 缓存24小时
                .withBody("", "text/plain");
        }

        // 遍历所有注册的路由
        for (const auto &route : routes_)
        {
            // 检查请求方法是否匹配
            if (route.method == request.getMethod())
            {
                std::unordered_map<std::string, std::string> pathParams;
                // 检查路径是否匹配
                if (matchPath(route.path, request.getPath(), pathParams))
                {
                    // 创建一个可修改的请求副本来设置路径参数
                    HttpRequest modifiableRequest = request;
                    modifiableRequest.setPathParams(pathParams);

                    // 检查这个路由是否需要验证
                    if (route.use_auth_middleware && middleware_)
                    {
                        // 使用中间件处理请求
                        return middleware_(modifiableRequest, route.handler);
                    }
                    else
                    {
                        // 直接调用处理函数
                        return route.handler(modifiableRequest);
                    }
                }
            }
        }
        // 如果没有API路由匹配，尝试作为静态文件请求处理
        if (request.getMethod() == "GET" && !static_dir_.empty())
        {
            return serveStaticFile(request.getPath());
        }
        // 如果没有匹配的路由和静态文件，返回404
        return HttpResponse::NotFound("Endpoint not found");
    }
    // 路径参数匹配和提取实现
    bool Router::matchPath(const std::string &pattern,
                               const std::string &path,
                               std::unordered_map<std::string, std::string> &params)
    {
        //清空参数映射
        params.clear();

        // 快速检查：如果没有参数且路径完全匹配，直接返回
        if (pattern.find('{') == std::string::npos)
        {
            return pattern == path;
        }

        // lambda函数，用于将路径分割成字符串数组
        auto splitPath = [](const std::string &str) -> std::vector<std::string>
        {
            if (str.empty() || str == "/") return {};
            
            std::vector<std::string> segments;
            segments.reserve(8); // 预分配一些空间
            
            size_t start = (str[0] == '/') ? 1 : 0;
            size_t pos = start;
            
            while (pos < str.length())
            {
                size_t next = str.find('/', pos);
                if (next == std::string::npos)
                {
                    if (pos < str.length())
                    {
                        segments.emplace_back(str.substr(pos));
                    }
                    break;
                }
                if (next > pos)
                {
                    segments.emplace_back(str.substr(pos, next - pos));
                }
                pos = next + 1;
            }
            return segments;
        };

        auto patternSegments = splitPath(pattern);//将pattern分段
        auto pathSegments = splitPath(path);//将path分段

        // 段数必须相同
        if (patternSegments.size() != pathSegments.size())
        {
            return false;
        }

        // 逐段匹配
        for (size_t i = 0; i < patternSegments.size(); ++i)
        {
            const std::string &patternSeg = patternSegments[i];
            const std::string &pathSeg = pathSegments[i];

            // 检查是否为参数段（以{开头并以}结尾）
            if (patternSeg.length() > 2 && patternSeg.front() == '{' && patternSeg.back() == '}')
            {
                // 提取参数名（去掉{}）
                std::string paramName = patternSeg.substr(1, patternSeg.length() - 2);
                params[paramName] = pathSeg;
            }
            else
            {
                // 精确匹配
                if (patternSeg != pathSeg)
                {
                    return false;
                }
            }
        }

        return true;
    }

    HttpResponse Router::serveStaticFile(const std::string &path)
    {
        std::string safe_path = path;
        
        // 增强的安全检查：防止目录遍历攻击
        if (safe_path.find("..") != std::string::npos ||
            safe_path.find("%2e%2e") != std::string::npos ||  // URL编码的..
            safe_path.find("%2E%2E") != std::string::npos ||  // URL编码的..
            safe_path.find("\\") != std::string::npos)        // Windows路径分隔符
        {
            LOG_WARN << "Path traversal attempt detected: " << path;
            return HttpResponse::Forbidden("Path traversal not allowed.");
        }

        // 确保路径以/开头
        if (!safe_path.empty() && safe_path[0] != '/')
        {
            safe_path = "/" + safe_path;
        }

        std::string full_path = static_dir_ + (safe_path == "/" ? "/index.html" : safe_path);

        std::ifstream file(full_path, std::ios::binary | std::ios::ate);
        if (!file)
        {
            LOG_DEBUG << "Static file not found: " << full_path;
            return HttpResponse::NotFound("Static file not found.");
        }

        // 获取文件大小
        std::streamsize file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        // 检查文件大小限制（防止内存耗尽）
        const std::streamsize MAX_FILE_SIZE = 50 * 1024 * 1024; // 50MB限制
        if (file_size > MAX_FILE_SIZE)
        {
            LOG_WARN << "File too large: " << full_path << " (" << file_size << " bytes)";
            return HttpResponse::InternalError("File too large");
        }

        // 预分配内存并读取文件
        std::string content;
        content.reserve(static_cast<size_t>(file_size));
        content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

        auto ext_pos = full_path.find_last_of('.');
        std::string mime_type = "application/octet-stream"; // 默认
        if (ext_pos != std::string::npos)
        {
            std::string ext = full_path.substr(ext_pos + 1);
            // 转换为小写进行MIME类型查找
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            auto it = MIME_TYPES.find(ext);
            if (it != MIME_TYPES.end())
            {
                mime_type = it->second;
            }
        }

        // 使用流式接口构建响应
        return HttpResponse::Ok()
            .withBody(content, mime_type)
            .withHeader("Cache-Control", "public, max-age=3600");
    }
}