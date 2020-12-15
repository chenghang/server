#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>

namespace myserver {

class ThreadPool {
public:
    ThreadPool(int numWorkers);
    ~ThreadPool();
    void pushJob(const std::function<void()>& job);

private:
    std::vector<std::thread> threads_;   //数组，存放线程
    std::mutex lock_;
    std::condition_variable cond_;
    std::queue<std::function<void()>> jobs_;     //线程池任务队列
    bool stop_;                      
};
}
#endif
