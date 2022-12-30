#ifndef LOG_DEF
#define LOG_DEF

#include <fstream>
#include <iostream>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "circulQueue.h"
#include <time.h>
#include <pthread.h>
#include <string>
#include <string.h>
#include <memory>
#include <stdarg.h>

/**
 * @brief Log level.
 *
 */
enum class logClass : uint8_t {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
};

/**
 * @brief A struct used to log the level and logContent.
 *
 */
struct logEventInfo {
    std::string logContent;
    logClass lgclass;
};

/**
 * @brief A async log tool. User will append log request to the message queue and working
 *        thread will pop request and write it to the log file.
 *
 */
class log {
public:

    static pthread_mutex_t m_createMutex; /// mutext used to do thread sync.

    /**
     * @brief Get the Instance object
     *
     * @param[in] fileName The log file name.
     * @return log* Handle of log tool.
     */
    static log* getInstance(const char *fileName)
    {
        if (m_instance == nullptr) {
            pthread_mutex_lock(&m_createMutex);
            if (m_instance == nullptr) {
                m_instance = new log(fileName, 60000);
            }
            pthread_mutex_unlock(&m_createMutex);
        }
        return m_instance;
    }

    /**
     * @brief Get the Instance object
     *
     * @return log* Handle of log tool.
     */
    static log* getInstance()
    {
        if (m_instance == nullptr) {
            pthread_mutex_lock(&m_createMutex);
            if (m_instance == nullptr) {
                m_instance = new log("/home/miyan/web/webserver/log/defaultLog.txt", 60000);
            }
            pthread_mutex_unlock(&m_createMutex);
        }
        return m_instance;
    }

    /**
     * @brief Get the filename.
     *
     * @return char* file name.
     */
    static char* getFilename()
    {
        return log::getInstance(nullptr)->m_fileName;
    }

private:
    static log* m_instance; /// single mode, the only instance's pointer.

public:

    char *m_fileName = nullptr; /// file name.
    std::ofstream m_file; /// file stream.
    int m_lineCnt = 0; /// The line cnt.
    msgQueue<std::string> m_queue; /// message queue.
    msgQueue<logClass> m_classQueue; /// class queue.
    pthread_mutex_t m_mutex = PTHREAD_MUTEX_INITIALIZER; /// mutex used to do thread sync.

    /**
     * @brief Helper function of working thread. Pop item from log queue and write it to the file.
     *
     */
    void workerHelper()
    {
        std::string event;
        logClass lgClss;
        while (m_queue.pop(event) && m_classQueue.pop(lgClss)) {
            write(event.c_str(), lgClss);
        }
    }

    /**
     * @brief Working thread function.
     *
     * @param arg 
     * @return void* 
     */
    static void * worker(void * arg)
    {
        log * ptr = log::getInstance(log::getFilename());
        ptr->workerHelper();
        return (void *)(0);
    }

    /**
     * @brief Push log content and log level to the queue.
     *
     * @param logContent Log content.
     * @param lgclass Log level.
     * @return true Write successfully.
     * @return false Write unsuccessfully.
     */
    bool writeLog(const char * logContent, logClass lgclass)
    {
        return m_queue.push(logContent) && m_classQueue.push(lgclass);
    }

    /**
     * @brief Push log content and log level to the queue.
     *
     * @param logContent Log content.
     * @param lgclass Log level.
     * @param ... variable parameters.
     * @return true Write successfully.
     * @return false Write unsuccessfully.
     */
    bool writeLog(logClass lgclass, const char * logContent,  ...)
    {
        memset(m_logBuf, '\0', 1024);
        va_list varList;
        va_start(varList, logContent);
        vsnprintf(m_logBuf, 1024, logContent, varList);
        va_end(varList);
        return m_queue.push(m_logBuf) && m_classQueue.push(lgclass);
    }

    /**
     * @brief Write content to the file according to the log level.
     *
     * @param logContent Log content.
     * @param lgclass Log level.
     */
    void write(const char * logContent, logClass lgclass)
    {

        time_t timer = time(NULL);
        struct tm *localtm = localtime(&timer);
        char timeInfo[30];
        sprintf(timeInfo, "_%d_%d_%d_%d_%d_%d", localtm->tm_year + 1900,
                localtm->tm_mon + 1, localtm->tm_mday, localtm->tm_hour,
                localtm->tm_min, localtm->tm_sec);
        
        pthread_mutex_lock(&m_mutex);
        if (m_lineCnt >= 50000) {
            m_lineCnt = 0;
            m_file.close();
            std::unique_ptr<char> newFileName(new char[100]);
            strcpy(newFileName.get(), m_fileName);
            strcat(newFileName.get(), timeInfo);
            m_file.open(newFileName.get(), std::ofstream::out | std::ofstream::app);
            if (!m_file.is_open()) {
                std::cout<<"not open"<<std::endl;
            }
        } 
        m_lineCnt++;
        m_file<<timeInfo;
        m_file<<"   ";
        switch (lgclass) {
            case logClass::DEBUG:
                m_file<<"[DEBUG]:";
                break;
            case logClass::INFO:
                m_file<<"[INFO]:";
                break;
            case logClass::WARNING:
                m_file<<"[WARNING]:";
                break;
            case logClass::ERROR:
                m_file<<"[ERROR]:";
                break;
            default:
                m_file<<"[NONE CLASS]:";
        }
        m_file<<logContent;
        m_file<<std::endl;
        m_file.flush();
        pthread_mutex_unlock(&m_mutex);
    }

public:
    /**
     * @brief Construct a new log object
     *
     * @param[in] fileName file name.
     * @param[in] queueSize log queue size.
     */
    log(const char *fileName, int queueSize): m_queue(queueSize), m_classQueue(queueSize)
    {
        m_fileName = new char[200];
        strcpy(m_fileName, fileName);
        m_file.open(m_fileName, std::ofstream::out | std::ofstream::app);
        if (!m_file.is_open()) {
            std::cout<<"not open"<<std::endl;
        }
    }

    /**
     * @brief Create worker thread.
     *
     * @return pthread_t working thread ID.
     */
    pthread_t init()
    {
        if (!m_threadHaveBeenCreated) {
            pthread_t tid;
            pthread_create(&tid, NULL, worker, NULL);
            m_threadHaveBeenCreated = true;
            return tid;
        }
        return 0;
    }

    /**
     * @brief Destroy the log object
     *
     */
    ~log()
    {
        m_file.close();
        delete m_fileName;
    }

    bool m_threadHaveBeenCreated = false; /// flag used to identify whether working thread has been created.
    char m_logBuf[1024]; /// log buffer.
};


#endif