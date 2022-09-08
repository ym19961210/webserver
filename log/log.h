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

enum class logClass : uint8_t {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
};

struct logEventInfo {
    std::string logContent;
    logClass lgclass;
};

class log {
public:

    static pthread_mutex_t m_createMutex;
    static log* getInstance(const char *fileName) // todo:bug here.
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

    static char* getFilename()
    {
        return log::getInstance(nullptr)->m_fileName;
    }

private:
    static log* m_instance;

public:

    char *m_fileName = nullptr;
    std::ofstream m_file;
    int m_lineCnt = 0;
    msgQueue<std::string> m_queue;
    msgQueue<logClass> m_classQueue;
    pthread_mutex_t m_mutex = PTHREAD_MUTEX_INITIALIZER;

    void workerHelper()
    {
        std::string event;
        logClass lgClss;
        while (m_queue.pop(event) && m_classQueue.pop(lgClss)) {
            write(event.c_str(), lgClss);
        }
    }

    static void * worker(void * arg)
    {
        std::cout<<log::getFilename()<<std::endl;
        log * ptr = log::getInstance(log::getFilename());
        ptr->workerHelper();
        return (void *)(0);
    }

    bool writeLog(const char * logContent, logClass lgclass)
    {
        return m_queue.push(logContent) && m_classQueue.push(lgclass);
    }

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
            std::unique_ptr<char> newFileName(new char[50]);
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
    log(const char *fileName, int queueSize): m_queue(queueSize), m_classQueue(queueSize)
    {
        m_fileName = new char[200];
        strcpy(m_fileName, fileName);
        std::cout<<m_fileName<<std::endl;
        m_file.open(m_fileName, std::ofstream::out | std::ofstream::app);
        if (!m_file.is_open()) {
            std::cout<<"not open"<<std::endl;
        }
    }

    pthread_t init()
    {
        pthread_t tid;
        pthread_create(&tid, NULL, worker, NULL);
        return tid;
    }

    ~log()
    {
        m_file.close();
        delete m_fileName;
    }
};


#endif