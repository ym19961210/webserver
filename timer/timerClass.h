#ifndef TIMER_CLASS
#define TIMER_CLASS

#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>


class timer;

/**
 * @brief Client socket and corresponding timer.
 *
 */
struct clientData {
    int socket;
    timer *curTimer;
};

/**
 * @brief Basic elements of sort list, every connection has one timer in the sort list.
 *
 */
class timer {
public:
    timer *pre = NULL; /// double direction link list previous node.
    timer *next = NULL; /// double direction link list next node.

    void (*m_dealFun)(clientData *);

    clientData *m_clientData = NULL; /// corresponding clientData.
    static int m_epollFd; /// epoll fd.

    time_t m_expireTime; /// expire time.
};


/**
 * @brief list sorting by the time when connection communicate last time.
 *
 */
class sort_list_timer {
public:
    /**
     * @brief Construct a new sort list timer object
     *
     */
    sort_list_timer():head(NULL), tail(NULL){};

    /**
     * @brief Destroy the sort list timer object
     *
     */
    ~sort_list_timer()
    {
        timer *tmp = head;
        while (tmp) {
            head = tmp->next;
            delete tmp;
            tmp = head;
        }
    }

    /**
     * @brief Add a timer element to the sort list.
     *
     * @param[in] targerTimer target timer.
     */
    void addTimer(timer *targerTimer);

    /**
     * @brief Adjust a timer, when timer is already in the list and data change happends.
     *
     * @param[in] targerTimer target timer.
     */
    void adjustTimer(timer *targerTimer);

    /**
     * @brief Delete a timer element to the sort list.
     *
     * @param[in] targerTimer target timer.
     */
    void deleteTimer(timer *targerTimer);

    /**
     * @brief This function will be called every X seconds.
     *
     */
    void tick();

    /**
     * @brief Get the Header of the list.
     *
     * @return timer* header.
     */
    timer *getHeader()
    {
        return head;
    }

private:
    timer *head = NULL; /// Head of the link list.
    timer *tail = NULL; /// Tail of the link list.

private:
    /**
     * @brief An internal helper function, used to add a timer to the list.
     *
     * @param targerTimer target timer.
     * @param headList Head of the list.
     */
    void addTimer(timer *targerTimer, timer *headList);
};

#endif