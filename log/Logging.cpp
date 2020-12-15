#include "Logging.h"
#include <assert.h>
#include "AsyncLogging.h"
#include <iostream>
#include <time.h>  
#include <sys/time.h> 
using namespace std;


static pthread_once_t once_control_ = PTHREAD_ONCE_INIT;
static AsyncLogging *AsyncLogger_;


void once_init()//回调函数
{
    AsyncLogger_ = new AsyncLogging(std::string("/mnt/hgfs/myshare/server/code/chenghang.log"));
    AsyncLogger_->start();   //启动后端线程
}

void output(const char* msg, int len)       //前端与后端的接口
{
	/*本函数使用初值为PTHREAD_ONCE_INIT的once_control变量保证once_init()函数
	在本进程执行序列中仅执行一次。在多线程编程环境下，尽管pthread_once()调用会出现在多个线程中，
	once_init()()函数仅执行一次，究竟在哪个线程中执行是不定的，是由内核调度来决定。*/
    pthread_once(&once_control_, once_init);
    AsyncLogger_->append(msg, len);
}

Logger::Impl::Impl(const char *fileName, int line)
  : stream_(),
    basename_(fileName),
    line_(line)
{
	formatTime();
}
void Logger::Impl::formatTime()
{
    struct timeval tv;
    time_t time;
    char str_t[26] = {0};
    gettimeofday (&tv, NULL);
    time = tv.tv_sec;
    struct tm* p_time = localtime(&time);   
    strftime(str_t, 26, "%Y-%m-%d %H:%M:%S\n", p_time);
    stream_ << str_t;
}

Logger::Logger(const char *fileName, int line)
  : impl_(fileName, line)
{
}

Logger::~Logger()
{
  impl_.stream_ << "  -- " << impl_.basename_ << ':' << impl_.line_ << '\n';
  const LogStream::Buffer& buf(stream().buffer());   //4000
  output(buf.data(), buf.length());      //这里输出缓冲区
}