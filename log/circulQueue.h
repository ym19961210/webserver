#include <pthread.h>
#include <string.h>


/**
 * @brief A message queue which is constructed with circle queue.
 *
 * @tparam T Type of message.
 */
template<typename T>
class msgQueue
{
public:
    pthread_mutex_t m_mutex = PTHREAD_MUTEX_INITIALIZER; /// mutex used to do thread sync.
    pthread_cond_t m_cond = PTHREAD_COND_INITIALIZER; /// condition variable used to do thread sync when queue is empty.
    pthread_cond_t m_condFull = PTHREAD_COND_INITIALIZER; /// condition variable used to do thread sync when queue is full.

    int curBack = -1; /// index of queue back.
    int curFront = -1; /// index of queue front.
    T *arr; /// pointer of array.
    int queueSize = 0; /// queue max size.
    int curSize = 0; /// current queue size.

    /**
     * @brief Construct a new msg Queue object
     *
     * @param size The size of queue.
     */
    msgQueue(int size)
    {
        arr = new T[size]();
        queueSize = size;
    }

    /**
     * @brief Destroy the msg Queue object
     *
     */
    ~msgQueue()
    {
        if (arr != nullptr) {
            delete[] arr;
            arr = nullptr;
        }
    }

    /**
     * @brief return the front item of queue.
     *
     * @param[in] item The item which is returned.
     * @return true return ok.
     * @return false No item in the queue.
     */
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

    /**
     * @brief return the back item of queue.
     *
     * @param[in] item The item which is returned.
     * @return true return ok.
     * @return false No item in the queue.
     */
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

    /**
     * @brief Return whether the queue is empty.
     *
     * @return true Is empty.
     * @return false Is not empty.
     */
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

    /**
     * @brief push a item to the back item of queue.
     * @note This function will block until the queue is ok to push item.
     *
     * @param[in] item The item which is returned.
     * @return true push ok.
     */
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

    /**
     * @brief pop a item from the queue.
     * @note This function will block until the queue is ok to pop item.
     *
     * @param[out] item The item which is returned.
     * @return true pop ok.
     */
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