#include "FileUtil.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

AppendFile::AppendFile(string filename)
  : fp_(fopen(filename.c_str(), "ae"))
{/*在打开文件流后，读取内容之前，调用setbuffer()可用来设置文件流的缓冲区。
	参数stream为指定的文件流，参数buf指向自定的缓冲区起始地址，参数size为缓冲区大小。*/
  setbuffer(fp_, buffer_, sizeof buffer_);
}

AppendFile::~AppendFile()
{
  fclose(fp_);
}

void AppendFile::append(const char* logline, const size_t len)
{
  size_t n = write(logline, len);
  size_t remain = len - n;
  while (remain > 0)    //没写完
  {
    size_t x = write(logline + n, remain);
    if (x == 0)
    {
      int err = ferror(fp_);
      if (err)
      {
        fprintf(stderr, "AppendFile::append() failed !\n");
      }
      break;
    }
    n += x;
    remain = len - n;
  }
}

void AppendFile::flush()//刷新fp_的缓冲区，就是将缓冲区的内容全部写入到文件描述符中
{
  fflush(fp_);     
}

size_t AppendFile::write(const char* logline, size_t len)
{
  return ::fwrite_unlocked(logline, 1, len, fp_);
}