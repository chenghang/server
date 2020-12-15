#ifndef __EPOLL_H__
#define __EPOLL_H__

#include <functional> // function
#include <vector> // vector
#include <memory> // shared_ptr
#include <sys/epoll.h> // epoll_event
#include "HttpRequest.h"
#include "ThreadPool.h"

#define MAXEVENTS 1024

namespace myserver {
class Epoll {
public:
    Epoll();
    ~Epoll();
	
    int add(int fd, HttpRequest* request, int events); // 注册新描述符
    int mod(int fd, HttpRequest* request, int events); // 修改描述符状态
    int del(int fd, HttpRequest* request, int events); // 从epoll中删除描述符
    int wait(int timeoutMs); // 等待事件发生, 返回活跃描述符数量
    void handleEvent(int listenFd, std::unique_ptr<ThreadPool>& threadPool, int eventsNum); // 调用事件处理函数
    
	
	void setOnConnection(const std::function<void()>& cb) { onConnection_ = cb; } // 设置新连接回调函数
    void setOnCloseConnection(const std::function<void(HttpRequest*)>& cb) { onCloseConnection_ = cb; } // 设置关闭连接回调函数
    void setOnRequest(const std::function<void(HttpRequest*)>& cb) { onRequest_ = cb; } // 设置处理请求回调函数
    void setOnResponse(const std::function<void(HttpRequest*)>& cb) { onResponse_ = cb; } // 设置响应请求回调函数

private: 
    int epollFd_;
    std::vector<struct epoll_event> events_;   //存储epoll检测到的就绪事件的epoll_event结构体
	
    std::function<void()> onConnection_;
    std::function<void(HttpRequest*)> onCloseConnection_;
    std::function<void(HttpRequest*)> onRequest_;
    std::function<void(HttpRequest*)> onResponse_;
}; 
} 
#endif
