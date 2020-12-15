#pragma once
#include "FileUtil.h"
#include "MutexLock.h"
#include "noncopyable.h"
#include <memory>
#include <string>
/*将日志信息写入到文件当中的类*/
class LogFile : noncopyable
{
public:
    LogFile(const std::string& basename, int flushEveryN = 1024);
    ~LogFile();

    void append(const char* logline, int len);     //外部接口1
    void flush();                                  //外部接口2

 private:
  void append_unlocked(const char* logline, int len);

  const std::string basename_;
  const int flushEveryN_;

  int count_;
  std::unique_ptr<MutexLock> mutex_;
  std::unique_ptr<AppendFile> file_;
};