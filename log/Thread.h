#pragma once
#include "CountDownLatch.h"
#include <functional>
#include "noncopyable.h"
#include <memory>
#include <pthread.h>
#include <string>

class Thread : noncopyable
{
 public:
  typedef std::function<void ()> ThreadFunc;

  explicit Thread(const ThreadFunc&, const std::string& name = std::string());
  ~Thread();

  void start();
  int join();

  bool started() const { return started_; }
  const std::string& name() const { return name_; }


 private:
  void setDefaultName();

  bool       started_;
  bool       joined_;
  pthread_t  pthreadId_;   //进程内唯一的，不同进程内可能相同。
  ThreadFunc func_;      //线程函数
  std::string     name_;   //线程名
  
  CountDownLatch latch_;
};