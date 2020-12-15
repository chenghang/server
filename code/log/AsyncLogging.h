#pragma once
#include "CountDownLatch.h"
#include "MutexLock.h"
#include "Thread.h"
#include "LogStream.h"
#include "noncopyable.h"
#include <functional>
#include <string>
#include <vector>

class AsyncLogging : noncopyable
{
 public:

  AsyncLogging(const std::string basename, int flushInterval = 2);

  ~AsyncLogging()
  {
    if (running_)
    {
      stop();
    }
  }

  void append(const char* logline, int len);

  void start()
  {
    running_ = true;
    thread_.start();//运行线程函数
    latch_.wait();
  }

  void stop()
  {
    running_ = false;
    cond_.notify();
    thread_.join();
  }

 private:

  // declare but not define, prevent compiler-synthesized functions
  AsyncLogging(const AsyncLogging&);  // ptr_container
  void operator=(const AsyncLogging&);  // ptr_container

  void threadFunc();

  typedef FixedBuffer<kLargeBuffer> Buffer; //4MB
  typedef std::vector<std::shared_ptr<Buffer>> BufferVector;
  typedef std::shared_ptr<Buffer> BufferPtr;

  const int flushInterval_;
  bool running_;            
  
  std::string basename_;
  Thread thread_;
  
  
  MutexLock mutex_;           
  Condition cond_;         
  
  BufferPtr currentBuffer_;// 当前的缓冲区
  BufferPtr nextBuffer_;// 预备的缓冲区，muduo异步日志中采用了双缓存技术
  BufferVector buffers_;// 缓冲区队列，每写完一个缓冲区，就把这个缓冲区挂到队列中，这些缓存都是等待待写入的文件
  CountDownLatch latch_;  //计数器
};