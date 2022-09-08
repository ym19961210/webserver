#ifndef TIMER_CLASS
#define TIMER_CLASS

#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>


class timer;

struct clientData {
    int socket;
    timer *curTimer;
};

class timer {
public:
    timer *pre = NULL;
    timer *next = NULL;

    void (*m_dealFun)(clientData *);

    clientData *m_clientData = NULL;
    static int m_epollFd;

    time_t m_expireTime;
};



class sort_list_timer {
public:
    sort_list_timer():head(NULL), tail(NULL){};

    ~sort_list_timer()
    {
        timer *tmp = head;
        while (tmp) {
            head = tmp->next;
            delete tmp;
            tmp = head;
        }
    }

    void addTimer(timer *targerTimer);
    void adjustTimer(timer *targerTimer);
    void deleteTimer(timer *targerTimer);
    void tick();

    timer *getHeader()
    {
        return head;
    }

private:
    timer *head = NULL;
    timer *tail = NULL;

private:
    void addTimer(timer *targerTimer, timer *headList);
};

#endif