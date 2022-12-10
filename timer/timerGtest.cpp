#include "gtest/gtest.h"
#include "timerClass.h"

#include <unistd.h>
// #include <string.h>



TEST(timerTest, basicInsert)
{
    sort_list_timer timerList;
    clientData data = {5, NULL};
    timer *tm = new timer();
    tm->m_clientData = &data;
    timerList.addTimer(tm);
    EXPECT_EQ(timerList.getHeader()->m_clientData->socket, 5);
}

TEST(timerTest, basicDel)
{
    sort_list_timer timerList;
    clientData data = {5, NULL};
    clientData anotherData = {15, NULL};
    timer *tm = new timer();
    tm->m_clientData = &data;
    timer *anotherTm = new timer();
    anotherTm->m_clientData = &anotherData;

    timerList.addTimer(tm);
    timerList.addTimer(anotherTm);

    timerList.deleteTimer(tm);

    EXPECT_EQ(timerList.getHeader(), anotherTm);
}


TEST(timerTest, basicAdjust)
{
    sort_list_timer timerList;
    clientData data = {5, NULL};
    clientData anotherData = {15, NULL};
    clientData otherData = {10, NULL};
    timer *tm = new timer();
    tm->m_clientData = &data;
    timer *anotherTm = new timer();
    anotherTm->m_clientData = &anotherData;
    timer *otherTm = new timer();
    otherTm->m_clientData = &otherData;

    tm->m_expireTime = 5;
    anotherTm->m_expireTime = 15;
    otherTm->m_expireTime = 10;

    timerList.addTimer(tm);
    timerList.addTimer(anotherTm);
    timerList.addTimer(otherTm);


    EXPECT_EQ(timerList.getHeader()->next->m_clientData->socket, 10);

    otherTm->m_expireTime = 20;
    timerList.adjustTimer(otherTm);
}