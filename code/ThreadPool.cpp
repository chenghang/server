#include "ThreadPool.h"

#include <iostream>
#include <cassert>

using namespace myserver;

ThreadPool::ThreadPool(int numWorkers): stop_(false)
{
    numWorkers = numWorkers <= 0 ? 1 : numWorkers;
    for(int i = 0; i < numWorkers; ++i)
        threads_.emplace_back(     
	[this]() {     //lambda表达式来书写线程函数，捕获当前类的this指针
            while(1) 
			{
                std::function<void()> func;
                {
                    std::unique_lock<std::mutex> lock(lock_); //注意要上锁！   
                    while(!stop_ && jobs_.empty())
                        cond_.wait(lock);      //线程等待在这个位置
                    if(jobs_.empty() && stop_) 
					{
                        // printf("[ThreadPool::ThreadPool] threadid = %lu return\n", pthread_self());
                        return;
                    }
                    func = jobs_.front();
                    jobs_.pop();
                }
                if(func) 
				{
                    // printf("[ThreadPool::ThreadPool] threadid = %lu get a job\n", pthread_self()/*std::this_thread::get_id()*/);
					func();
                    // printf("[ThreadPool::ThreadPool] threadid = %lu job finish\n", pthread_self()/*std::this_thread::get_id()*/);
                } 
            }
        });
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(lock_);//上锁！！！
        stop_ = true;
    } 
    cond_.notify_all();
    for(auto& thread: threads_)     //回收线程
        thread.join();
    // printf("[ThreadPool::~ThreadPool] threadpool is remove\n");
}

void ThreadPool::pushJob(const std::function<void()>& job)
{
	//printf("将此任务添加线程池任务队列！\n");
    {
        std::unique_lock<std::mutex> lock(lock_);//上锁！
        jobs_.push(job);
    }
    // printf("[ThreadPool::pushJob] push new job\n");
    cond_.notify_one();
}
