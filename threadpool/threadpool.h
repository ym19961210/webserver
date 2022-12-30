#ifndef THREAD_POOL
#define THREAD_POOL

#include <semaphore.h>
#include <list>
#include <pthread.h>
#include "../test.h"

/**
 * @brief Thread pool.
 *
 * @tparam T Type of thread.
 */
template<typename T>
class threadPool
{
public:
    int m_threadMaxNum; /// Max Thread number.
    int m_maxRequestNum; /// Max request number.

    /**
     * @brief Construct a new thread Pool object
     *
     * @param threadNum Max Thread number.
     * @param maxRequestNum Max request number.
     */
    threadPool(int threadNum = 8, int maxRequestNum = 10000);

    /**
     * @brief Destroy the thread Pool object
     *
     */
    ~threadPool();

    /**
     * @brief Append a request to the internal queue, when there is request in the queue, worker thread will be awaked to do business logic.
     *
     * @param request The request which is going to be added.
     * @return true Append successfully.
     * @return false Append unsuccessfully.
     */
    bool append(T *request);

private:
    /**
     * @brief Function which run in the working thread.
     *
     * @param arg parameters which working thread is using.
     * @return void* no use, which is required by the thread create process.
     */
    static void * worker(void * arg);

    /**
     * @brief Working function.
     *
     */
    void run();

public:
    sem_t m_workQueueCond; /// sem used to do thread sync.
    std::list<T*> m_requestQueue; /// request list.
    pthread_mutex_t m_mutex; /// mutex used to do thread sync.
    pthread_t* m_thread; /// Pointer of working thread array.
    log* m_logger; /// logger handle.
};

template<typename T>
threadPool<T>::threadPool(int threadNum, int maxRequestNum)
{
    if (threadNum <= 0 || maxRequestNum <= 0)
    {
        throw std::exception();
    }
    m_threadMaxNum = threadNum;
    m_maxRequestNum = maxRequestNum;
    pthread_mutex_init(&m_mutex, NULL);
    sem_init(&m_workQueueCond, 0, 0);


    m_thread = new pthread_t[m_threadMaxNum];
    if (!m_thread)
    {
        throw std::exception();
    }

    m_logger = log::getInstance("/home/miyan/web/webserver/log/ym.txt");


    for (uint16_t i = 0; i < m_threadMaxNum; i++) {
        if (pthread_create(m_thread + i, NULL, worker, this) != 0)
        {
            delete[] m_thread;
            throw std::exception();
        }
    }
    for (uint16_t i = 0; i < m_threadMaxNum; i++) {
        if (pthread_detach(m_thread[i]) != 0)
        {
            delete[] m_thread;
            throw std::exception();
        }
    }
}

template<typename T>
threadPool<T>::~threadPool()
{
    if (m_thread != nullptr) {
        delete[] m_thread;
    }
    sem_destroy(&m_workQueueCond);

    pthread_mutex_destroy(&m_mutex);
}

template<typename T>
bool threadPool<T>::append(T *request)
{
    if (static_cast<int>(m_requestQueue.size()) > m_maxRequestNum) {
        m_logger->writeLog("out of queue size", logClass::INFO);
        return false;
    }
    pthread_mutex_lock(&m_mutex);
    m_requestQueue.push_back(request);
    pthread_mutex_unlock(&m_mutex);
    sem_post(&m_workQueueCond);
    return true;
}

template<typename T>
void * threadPool<T>::worker(void * arg)
{
    threadPool *poolPtr = (threadPool*)arg; // why don't need to add <T> in threadPool?
    poolPtr->run();
    return poolPtr;
}

template<typename T>
void threadPool<T>::run()
{
    while (true) {
        sem_wait(&m_workQueueCond);
        pthread_mutex_lock(&m_mutex);
        if (m_requestQueue.empty()) {
            pthread_mutex_unlock(&m_mutex);
            continue;
        }
        T *request = m_requestQueue.front();
        m_requestQueue.pop_front();
        pthread_mutex_unlock(&m_mutex);
        if (request == nullptr) {
            continue;
        }
        RetParserState ret = request->Process();
        bool result = request->processWrite(ret);
        request->processWriteHelper(result);
    }
}


#endif
