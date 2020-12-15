#include "AsyncLogging.h"
#include "LogFile.h"
#include <stdio.h>
#include <assert.h>

using namespace std;

AsyncLogging::AsyncLogging(const string basename,
                           int flushInterval)
  : flushInterval_(flushInterval),
    running_(false),
    basename_(basename),
    thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),//初始化后端线程
    mutex_(),
    cond_(mutex_),
    currentBuffer_(new Buffer),//4000*1000
    nextBuffer_(new Buffer),
    buffers_(),
    latch_(1)
{
  currentBuffer_->bzero();
  nextBuffer_->bzero();
  buffers_.reserve(16);
}

void AsyncLogging::append(const char* logline, int len)  //将前端小缓冲区整理成后端大的缓冲块
{
  MutexLockGuard lock(mutex_);  //这里要上锁，相当于往缓冲区队列中存数据  
  if (currentBuffer_->avail() > len)
  {
    currentBuffer_->append(logline, len);
  }
  else      //当前缓冲区空间不够
  {
    buffers_.push_back(currentBuffer_);
    currentBuffer_.reset();

    if (nextBuffer_)    
    {
      currentBuffer_ = std::move(nextBuffer_);
    }
    else
    {
      currentBuffer_.reset(new Buffer); // Rarely happens
    }
    currentBuffer_->append(logline, len);
    cond_.notify();        //写满至少一块才去通知后端线程
}
}

#include <unistd.h>
void AsyncLogging::threadFunc()
{
  assert(running_ == true);
  latch_.countDown();   //      
  LogFile output(basename_);//将大的缓冲块添加到文件缓冲区，并刷新入文件
  BufferPtr newBuffer1(new Buffer);
  BufferPtr newBuffer2(new Buffer);
  newBuffer1->bzero();
  newBuffer2->bzero();
  BufferVector buffersToWrite;
  buffersToWrite.reserve(16);
  while (running_)
  {
    assert(newBuffer1 && newBuffer1->length() == 0);
    assert(newBuffer2 && newBuffer2->length() == 0);
    assert(buffersToWrite.empty());
    {
      MutexLockGuard lock(mutex_);   //上锁，交换缓冲区队列
      if (buffers_.empty()) 
      {
		/*如果buffers_缓冲队列中没有需要处理的缓存，就条件阻塞flushInterval_时间，
        期间如果有线程将写满的缓存块挂在buffers_缓冲队列中，就会唤醒，否则就是到时间自动唤醒*/
        cond_.waitForSeconds(flushInterval_);
      }
      buffers_.push_back(currentBuffer_);// 把当前正在使用的缓存块放入buffers_缓冲队列中
      currentBuffer_.reset();
	/*给当前缓冲块置新值，注意这里是将newBuffer1转换成右值，然后再赋给currentBuffer_,
	 相当于把newBuffer1的值移动给了currentBuffer_在这之后，newBuffer1就为空了，如果使用，就会报出内存溢出*/
      currentBuffer_ = std::move(newBuffer1);
      buffersToWrite.swap(buffers_);// 将buffers_缓冲队列置换过来处理，并交换一个空的buffers_缓冲队列
      if (!nextBuffer_)    
      {
        nextBuffer_ = std::move(newBuffer2);//将预备缓存也换成新的
      }
    }

    /*在上锁这段时间中，是不会产生新的缓存的，由于IO操作比较浪费时间，
	所以下面这一块很可能需要执行很长时间，这时在前端如果有很多线程在运行，
	并且不断向缓存中写入数据，所以等到下一次再来处理缓存时，会有比较大的缓存需要处理。*/
	assert(!buffersToWrite.empty());
	
	/*如果在buffers_缓冲队列中的缓存块个数大于25，就把前2个缓存块保存下来，其他都去掉。
	这是一种自我保护的代码，防止出现日志过多堆积*/
    if (buffersToWrite.size() > 25)
    {
      buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());// 只保留前2个缓存块
    }

    for (size_t i = 0; i < buffersToWrite.size(); ++i)
    {
      output.append(buffersToWrite[i]->data(), buffersToWrite[i]->length());//将缓存块队列中的缓存块内容写入文件当中
    }
	
	//如果buffersToWrite的尺寸小于2，那就说明nextBuffer_不是空的，就不会进入if (!newBuffer2)判断
    if (buffersToWrite.size() > 2)
    {
      buffersToWrite.resize(2);
    }
	
    if (!newBuffer1) //newBuffer1肯定为空
    {
      assert(!buffersToWrite.empty());
	  /*由于之前把newBuffer1的值移动给了currentBuffer_，所以如果再次调用就会出错，
      所以需要使用给newBuffer1赋另外一个值，然后把缓存中的数据清空*/
      newBuffer1 = buffersToWrite.back();
      buffersToWrite.pop_back();
      newBuffer1->reset();
    }

    if (!newBuffer2)
    {
      assert(!buffersToWrite.empty());
      newBuffer2 = buffersToWrite.back();
      buffersToWrite.pop_back();
      newBuffer2->reset();
    }
    buffersToWrite.clear();
    output.flush();
  }
  output.flush();
}