#include "HttpServer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Utils.h"
#include "Epoll.h"
#include "ThreadPool.h"
#include "Timer.h"
#include "/mnt/hgfs/myshare/server/code/log/Logging.h"

#include <iostream>
#include <functional> // bind
#include <assert.h> // assert
#include <cstring> // bzero
 
#include <unistd.h> // close, read
#include <sys/socket.h> // accept
#include <arpa/inet.h> // sockaddr_in
#include <netinet/in.h>

using namespace myserver;

HttpServer::HttpServer(int port, int numThread) //端口号、线程数目
    : port_(port),
      listenFd_(utils::createListenFd(port_)),
	  
      listenRequest_(new HttpRequest(listenFd_)),
      epoll_(new Epoll()),
      threadPool_(new ThreadPool(numThread)),
      timerManager_(new TimerManager())
{
    assert(listenFd_ >= 0);//assert的作用是先计算表达式 expression ，如果其值为假（即为0），那么它先向stderr打印一条出错信息，然后通过调用 abort 来终止程序运行。
    epoll_ -> setOnConnection(std::bind(&HttpServer::__acceptConnection, this));  //绑定类成员函数
    epoll_ -> setOnCloseConnection(std::bind(&HttpServer::__closeConnection, this, std::placeholders::_1));
    epoll_ -> setOnRequest(std::bind(&HttpServer::__doRequest, this, std::placeholders::_1));
    epoll_ -> setOnResponse(std::bind(&HttpServer::__doResponse, this, std::placeholders::_1));
}

HttpServer::~HttpServer()
{
}

void HttpServer::run()
{
    // 注册监听套接字到epoll（可读事件，ET模式）
    epoll_ -> add(listenFd_, listenRequest_.get(), (EPOLLIN | EPOLLET));
    while(1) 
	{
        int timeMS = timerManager_ -> getNextExpireTime();
        int eventsNum = epoll_ -> wait(timeMS);
        if(eventsNum > 0) 
		{
            epoll_ -> handleEvent(listenFd_, threadPool_, eventsNum);
        }
        timerManager_ -> handleExpireTimers();   
    }
}

void HttpServer::__acceptConnection()
{
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(struct sockaddr_in));
	socklen_t client_addr_len = sizeof(client_addr);
	int acceptFd = 0;
    while(1) 
	{
        acceptFd = accept4(listenFd_,(struct sockaddr *)&client_addr,&client_addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
		LOG << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":"<< ntohs(client_addr.sin_port);
        if(acceptFd == -1) 
		{
            if(errno == EAGAIN)
                break;
            printf("[HttpServer::__acceptConnection] accept : %s\n", strerror(errno));
            break;
        }
        HttpRequest* request = new HttpRequest(acceptFd);
        timerManager_ -> addTimer(request, CONNECT_TIMEOUT, std::bind(&HttpServer::__closeConnection, this, request));
        epoll_ -> add(acceptFd, request, (EPOLLIN | EPOLLONESHOT | EPOLLET));
    }
}

void HttpServer::__closeConnection(HttpRequest* request)
{
    int fd = request -> fd();
    if(request -> isWorking()) {
        return;
    }
    timerManager_ -> delTimer(request);
    epoll_ -> del(fd, request, 0);
    delete request;// 释放该套接字占用的HttpRequest资源，在析构函数中close(fd)
    request = nullptr;
}

void HttpServer::__doRequest(HttpRequest* request)
{
    timerManager_ -> delTimer(request);  //删除计时器
    assert(request != nullptr);
    int fd = request -> fd();

    int readErrno;
    int nRead = request -> read(&readErrno);

    // read返回0表示客户端断开连接
    if(nRead == 0) 
	{
        request -> setNoWorking();
        __closeConnection(request);
        return; 
    }

    // 非EAGAIN错误，断开连接
    if(nRead < 0 && (readErrno != EAGAIN)) 
	{
        request -> setNoWorking();
        __closeConnection(request);
        return; 
    }

    // EAGAIN错误则释放线程使用权，并监听下次可读事件epoll_ -> mod(...)
    /*if(nRead < 0 && readErrno == EAGAIN) //没有数据可读
	{
		//EPOLLONESHOT一个socket连接在任意时刻都只被一个线程处理
        epoll_ -> mod(fd, request, (EPOLLIN | EPOLLONESHOT));//重置
        request -> setNoWorking();
        timerManager_ -> addTimer(request, CONNECT_TIMEOUT, std::bind(&HttpServer::__closeConnection, this, request));
        return;
    }*/
	while(nRead < 0 && readErrno == EAGAIN)
	{
		nRead = request -> read(&readErrno);
	}

    // 解析报文出错，断开连接
    if(!request -> parseRequest()) 
	{
        HttpResponse response(400, "", false);  // 生成400报文
        request -> appendOutBuffer(response.makeResponse());  //添加到输出缓冲区

        int writeErrno;
        request -> write(&writeErrno);
        request -> setNoWorking();
        __closeConnection(request); 
        return; 
    }

    // 解析完成
    if(request -> parseFinish()) 
	{
        HttpResponse response(200, request -> getPath(), request -> keepAlive());
        request -> appendOutBuffer(response.makeResponse());   //添加到输出缓冲区
        epoll_ -> mod(fd, request, (EPOLLIN | EPOLLOUT | EPOLLONESHOT | EPOLLET));   //这里关注了可写事件
    }
}

void HttpServer::__doResponse(HttpRequest* request)
{
    timerManager_ -> delTimer(request);   //删除计时器
    assert(request != nullptr);
    int fd = request -> fd();

    int toWrite = request -> writableBytes();  

    if(toWrite == 0) //输出缓冲区没有空间
	{
        epoll_ -> mod(fd, request, (EPOLLIN | EPOLLONESHOT | EPOLLET));//只关注可读事件即可
        request -> setNoWorking();
        timerManager_ -> addTimer(request, CONNECT_TIMEOUT, std::bind(&HttpServer::__closeConnection, this, request));
        return;
    }

    int writeErrno;
    int ret = request -> write(&writeErrno);

    if(ret < 0 && writeErrno == EAGAIN) //内核缓冲区满
	{
        epoll_ -> mod(fd, request, (EPOLLIN | EPOLLOUT | EPOLLONESHOT | EPOLLET));
        return;
    }

    // 非EAGAIN错误，断开连接
    if(ret < 0 && (writeErrno != EAGAIN)) 
	{
        request -> setNoWorking();
        __closeConnection(request);
        return; 
    }

    if(ret == toWrite) //写满
	{
        if(request -> keepAlive())    //长连接
		{
            request -> resetParse();  //重置连接信息
            epoll_ -> mod(fd, request, (EPOLLIN | EPOLLONESHOT | EPOLLET));
            request -> setNoWorking();
            timerManager_ -> addTimer(request, CONNECT_TIMEOUT, std::bind(&HttpServer::__closeConnection, this, request));
        } 
		else //短连接，关闭
		{
            request -> setNoWorking();
            __closeConnection(request);
        }
        return;
    }
   //没写满，等待下次就绪事件来临
    epoll_ -> mod(fd, request, (EPOLLIN | EPOLLOUT | EPOLLONESHOT | EPOLLET));
    request -> setNoWorking();
    timerManager_ -> addTimer(request, CONNECT_TIMEOUT, std::bind(&HttpServer::__closeConnection, this, request));
    return;
}
