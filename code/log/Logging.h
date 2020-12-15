#pragma once
#include "LogStream.h"
#include <pthread.h>
#include <string.h>
#include <string>
#include <stdio.h>

class AsyncLogging;

class Logger
{
public:
    Logger(const char *fileName, int line);
	 ~Logger();
    LogStream& stream() { return impl_.stream_; }    //该类核心

private:
    class Impl
    {
    public:
        Impl(const char *fileName, int line);
        void formatTime();

        LogStream stream_;
        int line_;
        std::string basename_;
    };
    Impl impl_;
};

#define LOG Logger(__FILE__, __LINE__).stream()//最终LOG表示一个Logstream对象