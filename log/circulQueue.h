#include <pthread.h>
#include <string.h>

template<typename T>
class msgQueue
{
public:
    pthread_mutex_t m_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t m_cond = PTHREAD_COND_INITIALIZER;
    pthread_cond_t m_condFull = PTHREAD_COND_INITIALIZER;

    int curBack = -1;
    int curFront = -1;
    T *arr;
    int queueSize = 0;
    int curSize = 0;

    msgQueue(int size)
    {
        arr = new T[size]();
        queueSize = size;
    }

    ~msgQueue()
    {
        if (arr != nullptr) {
            delete[] arr;
            arr = nullptr;
        }
    }

    bool front(T &item)
    {
        pthread_mutex_lock(&m_mutex);
        if (curSize == 0) {
            pthread_mutex_unlock(&m_mutex);
            return false;
        }
        item = arr[curFront + 1];
        pthread_mutex_unlock(&m_mutex);
        return true;
    }

    bool back(T &item)
    {
        pthread_mutex_lock(&m_mutex);
        if (curSize == 0) {
            pthread_mutex_unlock(&m_mutex);
            return false;
        }
        item = arr[curBack];
        pthread_mutex_unlock(&m_mutex);
        return true;
    }

    bool empty() const
    {
        pthread_mutex_lock(&m_mutex);
        if (curSize == 0) {
            pthread_mutex_unlock(&m_mutex);
            return true;
        }
        pthread_mutex_unlock(&m_mutex);
        return false;
    }

    bool push(T item)
    {
        pthread_mutex_lock(&m_mutex);
        while (curSize >= queueSize) {
            pthread_cond_wait(&m_condFull, &m_mutex);
        }
        curBack = (curBack + 1) % queueSize;
        arr[curBack] = item;
        curSize++;
        pthread_cond_broadcast(&m_cond);
        pthread_mutex_unlock(&m_mutex);
        return true;
    }

    bool pop(T &item)
    {
        pthread_mutex_lock(&m_mutex);
        while (curSize <= 0) {
            pthread_cond_wait(&m_cond, &m_mutex);
        }
        
        curFront = (curFront + 1) % queueSize;
        item = arr[curFront];
        /// The thread which is waked will decrease the condition curSize. Note this thread get the mutex!
        /// So other thread which had been waked will keep a state that waked by cond but blocked by the mutex.
        curSize--;
        pthread_cond_broadcast(&m_condFull);
        pthread_mutex_unlock(&m_mutex);
        return true;
    }
};