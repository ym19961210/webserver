#ifndef THREAD_POOL
#define THREAD_POOL

#include <semaphore.h>
#include <list>
#include <pthread.h>


template<typename T>
class threadPool
{
public:
    int m_threadNum;
    int m_threadMaxNum;
    int m_maxRequestNum;
    threadPool(int threadNum = 8, int maxRequestNum = 10000);
    ~threadPool();
    bool append(T *request);

private:
    static void * worker(void * arg);
    void run();

public:
    sem_t m_workQueueCond;
    std::list<T*> m_requestQueue;
    pthread_mutex_t m_mutex;
    pthread_t* m_thread;
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
    if (m_requestQueue.size() > m_maxRequestNum) {
        std::cout<<"out of queue size"<<std::endl;
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
        request->Process();
    }
}


#endif
