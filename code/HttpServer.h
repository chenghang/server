#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include <memory> // unique_ptr
#include <mutex>
#include "HttpRequest.h"
#include "Epoll.h"
#include "ThreadPool.h"
#include "Timer.h"

#define TIMEOUTMS -1 // epoll_wait等待超时时间，-1表示不设超时
#define CONNECT_TIMEOUT 500 // 连接默认超时时间
#define NUM_WORKERS 4 // 线程池大小

namespace myserver 
{
class HttpServer 
{
public:
    HttpServer(int port, int numThread);
    ~HttpServer();
    void run(); // 启动HTTP服务器
    
private:
    void __acceptConnection(); // 接受新连接
    void __closeConnection(HttpRequest* request); // 关闭连接
    void __doRequest(HttpRequest* request); // 解析HTTP请求报文，并生成响应报文存入用户缓冲区，这个函数由线程池调用
    void __doResponse(HttpRequest* request);  //将用户缓冲区数据存入内核缓冲区

private:

    int port_; // 监听端口
    int listenFd_; // 监听套接字
	 
	//以下全部用智能指针实现
    std::unique_ptr<HttpRequest> listenRequest_; // 监听套接字的HttpRequest实例
    std::unique_ptr<Epoll> epoll_; // epoll实例
    std::unique_ptr<ThreadPool> threadPool_; // 线程池
    std::unique_ptr<TimerManager> timerManager_; // 定时器管理器
}; 
} 

#endif
