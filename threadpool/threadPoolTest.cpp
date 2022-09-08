#include "gtest/gtest.h"
#include "threadpool.h"
#include <memory>
#include <unistd.h>

class testWork
{
public:
    bool flag = false;
    void Process()
    {
        flag = true;
    }
};

TEST(test_threadPool, test0)
{
    std::unique_ptr<threadPool<testWork> > ptr(new threadPool<testWork>(8, 10000));
    std::unique_ptr<testWork> requestPtr(new testWork());
    std::cout<<requestPtr->flag<<std::endl;
    ptr->append(requestPtr.get());
    sleep(2); // block thread will be waked after append. The google test process need to yield CPU for the worker thread.
    EXPECT_EQ(requestPtr->flag, true);
    std::cout<<requestPtr->flag<<std::endl;
}
