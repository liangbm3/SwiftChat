#pragma once
#include <string>
#include <functional>
#include <unordered_map>
#include "utils/thread_pool.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"

namespace http
{
    class HttpServer
    {
    public:
        //请求处理函数类型：接收HttpRequest对象的引用，返回HttpResponse对象
        using RequestHandler = std::function<HttpResponse(const HttpRequest &)>;
        explicit HttpServer(int port);
        ~HttpServer();
        //路由注册函数，为指定路径和方法添加处理函数
        void addHandler(const std::string &path,const std::string &method, RequestHandler handler);
        void run();//启动服务器
        void stop();//停止服务器
        void setStaticDirectory(const std::string &dir); // 设置静态文件目录

    private:
        int port_;//服务器监听的端口
        int server_fd_;//服务器的主监听套接字
        bool running_;//标志位，用于控制run()函数的循环
        utils::ThreadPool thread_pool_;//线程池对象
        //核心路由表，外层的键是路径，内层的键是HTTP方法，值为处理函数
        std::unordered_map<std::string,std::unordered_map<std::string, RequestHandler>> handlers_;
        std::string static_dir_;//存放静态文件的根目录

        void handleClient(int client_fd);//处理单个客户端连接的核心逻辑
        //提供静态文件服务的具体实现
        void serveStaticFile(const std::string &path, HttpResponse &response);

    };
}