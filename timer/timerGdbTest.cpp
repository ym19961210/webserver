#include "timerClass.h"
#include <iostream>
#include <unistd.h>

static sort_list_timer timerList;

int main()
{
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

    std::cout<<timerList.getHeader()->m_clientData->socket<<std::endl;
    std::cout<<timerList.getHeader()->next->m_clientData->socket<<std::endl;
    std::cout<<timerList.getHeader()->next->next->m_clientData->socket<<std::endl;
    // EXPECT_EQ(timerList.getHeader()->next->m_clientData->socket, 10);

    // delete tm;
    // delete anotherTm;
    // delete otherTm;
}