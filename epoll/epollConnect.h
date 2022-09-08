#ifndef EPOLL_CONNECT
#define EPOLL_CONNECT

#include "public_def/returnValue.h"
#include "httpParser/httpResponse.h"
#include "httpParser/httpParser.h"
#include <sys/types.h>
#include <sys/epoll.h>
#include <memory>
#include "threadpool/threadpool.h"
#include "log/log.h"

#define SERVER_PORT         (8001)
#define EPOLL_MAX_NUM       (2048)
#define BUFFER_MAX_LEN      (4096)

class epollConnect {
public:
    epollConnect();
    ~epollConnect();
    RetValue run();
private:
    int m_byteHadRead = 0;
    int m_byteHadReadTotal = 0;
    int m_epollFd = 0;
    httpResponser *m_response = nullptr;
    std::unique_ptr<httpParser> m_parser;
    std::unique_ptr<threadPool<httpParser> > m_threadPool;
    log* m_logger;
};

#endif