#pragma once

#include "Condition.h"
#include "MutexLock.h"

#include "noncopyable.h"

/*将条件变量和锁两个类进一步封装，因为条件变量必须与互斥锁一起用才有价值
 *设置一个计数器，计数值为count_，如果count_变为0，就可以满足条件，然后解除条件等待
 *既可用于所有子线程等待主线程发起起跑
 *也可用于主线程等待子线程初始化完毕才开始工作*/
class CountDownLatch : noncopyable
{
public:
    explicit CountDownLatch(int count);
    void wait();
    void countDown();
    int getCount() const;

private:
    mutable MutexLock mutex_;
    Condition condition_;
    int count_;
};