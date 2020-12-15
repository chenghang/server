#include "Epoll.h"
#include "HttpRequest.h"
#include "ThreadPool.h"

#include <iostream>
#include <cassert>
#include <cstring> // perror

#include <unistd.h> // close

using namespace myserver;

Epoll::Epoll() :epollFd_(epoll_create1(EPOLL_CLOEXEC)),  events_(MAXEVENTS)
{
    assert(epollFd_ >= 0);
}

Epoll::~Epoll()
{
    close(epollFd_);
}

int Epoll::add(int fd, HttpRequest* request, int events)
{
    struct epoll_event event;
    event.data.ptr = (void*)request; 
    event.events = events;
	
    int ret = epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event);//操作epoll的内核事件表
    return ret;    
}

int Epoll::mod(int fd, HttpRequest* request, int events)
{
    struct epoll_event event;
    event.data.ptr = (void*)request; 
    event.events = events;
	
    int ret = epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event);
    return ret;
}

int Epoll::del(int fd, HttpRequest* request, int events)
{
    struct epoll_event event;
    event.data.ptr = (void*)request; 
    event.events = events;
	
    int ret = ::epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &event);
    return ret;
}

int Epoll::wait(int timeoutMs)
{
    int eventsNum = epoll_wait(epollFd_, &epoll[0], MAXEVENTS, timeoutMs);
    if(eventsNum == 0) {
        printf("[Epoll::wait] nothing happen, epoll timeout\n");
    } 
	else if(eventsNum < 0) {
        printf("[Epoll::wait] epoll : %s\n", strerror(errno));
    }
    return eventsNum;
}

void Epoll::handleEvent(int listenFd, std::unique_ptr<ThreadPool>& threadPool, int eventsNum)
{
    assert(eventsNum > 0);
    for(int i = 0; i < eventsNum; ++i) 
	{
        HttpRequest* request = (HttpRequest*)(events_[i].data.ptr); 
        int fd = request -> fd();  //获得就绪连接的文件描述符
        if(fd == listenFd) 
		{
            // 新连接回调函数
            onConnection_();
        } 
		else 
		{
            // 排除错误事件
            if((events_[i].events & EPOLLERR) ||
               (events_[i].events & EPOLLHUP) ||
               (!events_[i].events & EPOLLIN)) 
			{
                request -> setNoWorking();
                onCloseConnection_(request);    // 出错则关闭连接
            } 
			else if(events_[i].events & EPOLLIN) //可读
			{   
				//printf("这是一个可写就绪事件，准备往线程池任务队列中添加一个读任务！\n");
                request -> setWorking();
                threadPool -> pushJob(std::bind(onRequest_, request));
            } 
			else if(events_[i].events & EPOLLOUT) //可写
			{   
				//printf("这是一个可读就绪事件，准备往线程池任务队列中添加一个写任务！\n");
                request -> setWorking();
                threadPool -> pushJob(std::bind(onResponse_, request));
            } 
			else {
                printf("[Epoll::handleEvent] unexpected event！\n");
            }
        }
    }
    return;
}
