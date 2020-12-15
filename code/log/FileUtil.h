#pragma once

#include "noncopyable.h"
#include <string>
/*AppendFile这个类主要是打开一个文件，建立文件的缓冲，然后对文件进行追加内容等操作*/
class AppendFile : noncopyable
{
public:
    explicit AppendFile(std::string filename);
    ~AppendFile();
    void append(const char* logline, const size_t len);  //外部接口1
    void flush();											//外部接口2

private:
    size_t write(const char* logline, size_t len);
    FILE* fp_;
    char buffer_[64*1024];      //64k，fp_的缓存
};