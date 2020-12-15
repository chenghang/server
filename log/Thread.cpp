#include "Thread.h"
#include <memory>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>
#include <stdint.h>
#include <assert.h>

#include <iostream>
using namespace std;

// 为了在线程中保留name,tid这些数据
struct ThreadData
{
  typedef Thread::ThreadFunc ThreadFunc;
  ThreadFunc func_;
  string name_;
  CountDownLatch* latch_;

  ThreadData(const ThreadFunc& func,
             const string& name,
             CountDownLatch* latch)
    : func_(func),
      name_(name),
      latch_(latch)
  { }

  void runInThread()   //执行线程函数
  {
    latch_->countDown();
    latch_ = NULL;
    func_();    //运行函数
  }
};

void* startThread(void* obj)   //子线程启动函数
{
  ThreadData* data = static_cast<ThreadData*>(obj);
  data->runInThread();
  delete data;
  return NULL;
}



Thread::Thread(const ThreadFunc& func, const string& n)
  : started_(false),
    joined_(false),
    pthreadId_(0),
    func_(func),
    name_(n),
    latch_(1)
{
  setDefaultName();
}

Thread::~Thread()
{
  if (started_ && !joined_)
  {
    pthread_detach(pthreadId_);
  }
}

void Thread::setDefaultName()
{
  if (name_.empty())
  {
    char buf[32];
    snprintf(buf, sizeof buf, "Thread");
    name_ = buf;
  }
}

void Thread::start()   //启动线程，在logging.cpp中调用
{
  assert(!started_);
  started_ = true;

  ThreadData* data = new ThreadData(func_, name_, &latch_);//管理一个线程的信息
  if (pthread_create(&pthreadId_, NULL, &startThread, data))  
  {
    started_ = false;
    delete data;
  }
  else   //创建成功返回0
  {
    latch_.wait();      //主线程等待子线程初始化完毕才开始工作
  }
}

int Thread::join()
{
    assert(started_);
    assert(!joined_);
    joined_ = true;
    return pthread_join(pthreadId_, NULL);
}