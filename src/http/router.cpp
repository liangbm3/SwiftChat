#include "router.hpp"
#include <fstream>
#include <sstream>
#include "utils/logger.hpp"

namespace http
{
    // 初始化静态MIME类型映射表
    const std::unordered_map<std::string, std::string> Router::MIME_TYPES = {
        {"html", "text/html"},
        {"css", "text/css"},
        {"js", "application/javascript"},
        {"json", "application/json"},
        {"png", "image/png"},
        {"jpg", "image/jpeg"},
        {"jpeg", "image/jpeg"},
        {"gif", "image/gif"},
        {"svg", "image/svg+xml"},
        {"ico", "image/x-icon"},
        {"txt", "text/plain"}};

    // 构造函数
    Router::Router() : static_dir_("./static") {}

    void Router::addHandler(const Route &route)
    {
        routes_.push_back(route);
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

        // lambda函数，用于将路径分割成字符串数组
        auto splitPath = [](const std::string &str) -> std::vector<std::string>
        {
            std::vector<std::string> segments;
            std::stringstream ss(str);
            std::string segment;
            while (std::getline(ss, segment, '/'))
            {
                if (!segment.empty())
                {
                    segments.push_back(segment);
                }
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
        // 基础安全检查：防止目录遍历攻击
        if (safe_path.find("..") != std::string::npos)
        {
            return HttpResponse::Forbidden("Path traversal not allowed.");
        }

        std::string full_path = static_dir_ + (path == "/" ? "/index.html" : path);

        std::ifstream file(full_path, std::ios::binary);
        if (!file)
        {
            return HttpResponse::NotFound("Static file not found.");
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();

        auto ext_pos = full_path.find_last_of('.');
        std::string mime_type = "application/octet-stream"; // 默认
        if (ext_pos != std::string::npos)
        {
            std::string ext = full_path.substr(ext_pos + 1);
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