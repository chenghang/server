#include "Buffer.h"
#include <cstring> // perror
#include <iostream>
#include <unistd.h> // write
#include <sys/uio.h> // readv

using namespace myserver;
/*
struct iovec {
    ptr_t iov_base; //Starting address 
    size_t iov_len; //Length in bytes 
};
struct iovec定义了一个向量元素。通常，这个结构用作一个多元素的数组。对于每一个传输的元素，
指针成员iov_base指向一个缓冲区，这个缓冲区是存放的是readv所接收的数据或是writev将要发送的数据。
成员iov_len在各种情况下分别确定了接收的最大长度以及实际写入的长度。


*/
ssize_t Buffer::readFd(int fd, int* savedErrno)
{
    // 保证一次读到足够多的数据
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writable = writableBytes();
    vec[0].iov_base = __begin() + writerIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);
    const ssize_t n = readv(fd, vec, 2);
    if(n < 0) {
        printf("[Buffer:readFd]fd = %d readv : %s\n", fd, strerror(errno));
        *savedErrno = errno;
    } 
    else if(static_cast<size_t>(n) <= writable)
        writerIndex_ += n;
    else {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }

    return n;
}

ssize_t Buffer::writeFd(int fd, int* savedErrno)
{
    size_t nLeft = readableBytes();
    char* bufPtr = __begin() + readerIndex_;
    ssize_t n;
    if((n = write(fd, bufPtr, nLeft)) <= 0) 
	{
        if(n < 0 && errno == EINTR)
            return 0;
        else  
		{
            printf("[Buffer:writeFd]fd = %d write : %s\n", fd, strerror(errno));
            *savedErrno = errno;
            return -1;
        }
    } 
	else {
        readerIndex_ += n;
        return n;
    }
}
