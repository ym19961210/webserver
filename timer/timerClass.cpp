#include "timerClass.h"
#include <iostream>
#include <time.h>

int timer::m_epollFd = 0;

void sort_list_timer::addTimer(timer *targerTimer, timer *headList)
{
    timer *prev = headList;
    timer *tmp = headList->next;

    while (tmp) {
        if (targerTimer->m_expireTime < tmp->m_expireTime) {
            prev->next = targerTimer;
            targerTimer->pre = prev;
            targerTimer->next = tmp;
            tmp->pre = targerTimer;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }

    if (!tmp) {
        prev->next = targerTimer;
        targerTimer->pre = prev;
        targerTimer->next = NULL;
        tail = targerTimer;
    }
}

void sort_list_timer::addTimer(timer *targerTimer)
{
    if (targerTimer == NULL) {
        return;
    }

    if (head == NULL) {
        head = tail = targerTimer;
        return;
    }

    if (targerTimer->m_expireTime < head->m_expireTime) {
        targerTimer->next = head;
        head->pre = targerTimer;
        targerTimer->pre = NULL;
        head = targerTimer;
        return;
    }

    addTimer(targerTimer, head);
}

void sort_list_timer::adjustTimer(timer *targerTimer)
{
    if (targerTimer == NULL) {
        return;
    }

    timer *tmp = targerTimer->next;
    if (!tmp || targerTimer->m_expireTime < tmp->m_expireTime) {
        return;
    }


    if (targerTimer == head) {
        head = head->next;
        head->pre = NULL;
        targerTimer->next = NULL;
        addTimer(targerTimer, head);
    } else {
        targerTimer->pre->next = tmp;
        tmp->pre = targerTimer->pre;
        addTimer(targerTimer, head);
    }
}

void sort_list_timer::deleteTimer(timer *targerTimer)
{
    if (targerTimer == NULL) {
        return;
    }

    if (head == targerTimer && targerTimer == tail) {
        delete targerTimer;
        head = NULL;
        tail = NULL;
    }

    if (targerTimer == head) {
        head = head->next;
        head->pre = NULL;
        targerTimer->next = NULL;
        delete targerTimer;
        return;
    }

    if (targerTimer == tail) {
        tail = tail->pre;
        tail->next = NULL;
        targerTimer->pre = NULL;
        delete targerTimer;
        return;
    }

    timer *tmp = targerTimer->next;
    targerTimer->pre->next = tmp;
    tmp->pre = targerTimer->pre;
    delete targerTimer;
}

void sort_list_timer::tick()
{
    time_t curTime = time(NULL);

    timer *tmp = head;

    while (tmp) {
        if (tmp->m_expireTime > curTime) {
            break;
        }


        (*(tmp->m_dealFun))(tmp->m_clientData);
        head = head->next;
        if (head) {
            head->pre = NULL;
        }
        delete tmp;
        tmp = head;
    }
}