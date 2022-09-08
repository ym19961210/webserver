#include "gtest/gtest.h"
#include "circulQueue.h"

#include <unistd.h>
#include <string.h>
#include <pthread.h>

class testWork
{
public:
    static int m_test;
    std::string m_str;
    testWork() = default;
    testWork(std::string m)
    {
        m_test++;
        m_str = m;
    }
};

int testWork::m_test = 0;

void * worker(void *arg)
{
    msgQueue<testWork> q = *((msgQueue<testWork>*)arg);
    testWork item;
    std::cout<<"pop begin"<<std::endl;
    while (1) {
        q.pop(item);
        std::cout<<item.m_str<<std::endl;
    }
    return (void *)(0);
}

TEST(test_log, test0)
{
    msgQueue<testWork> f(5);
    testWork item("test");
    f.push(item);
    testWork item1("test1");
    f.front(item1);
    EXPECT_EQ("test", item1.m_str);
    testWork item2("test3");
    f.push(item2);
    f.pop(item1);
    f.pop(item1);
    EXPECT_EQ("test3", item1.m_str);
}
