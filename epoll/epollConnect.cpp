#include "epollConnect.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <iostream>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include <signal.h>
#include <string>
#include "../test.h"

clock_t tm = 0;
clock_t tmLastValue = 0;
int *u_pipe;

pthread_mutex_t log::m_createMutex = PTHREAD_MUTEX_INITIALIZER;
log* log::m_instance = nullptr;

void callBackFunction(clientData *data)
{
    epoll_ctl(timer::m_epollFd, EPOLL_CTL_DEL, data->socket, NULL);
    close(data->socket);

    log *lger = log::getInstance();
    lger->writeLog(logClass::INFO, "socket close:%d", data->socket);
}

void sig_handler(int sig)
{
    if (sig == SIGALRM || sig == SIGTERM) {
        int msg = sig;
        send(u_pipe[1], (char *)(&msg), 1, 0);
    }
}

void addSig(int sig, void(handle)(int))
{
    struct sigaction action;
    memset(&action, '\0', sizeof(action));
    sigset_t set;
    sigfillset(&set);
    action.sa_handler = handle;
    action.sa_mask = set;
    sigaction(sig, &action, NULL);
}

int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

RetValue epollConnect::init()
{
    // init the logger
    m_logger = log::getInstance("");
    
    // init Httpresponse
    m_response = new httpResponser();

    //init thread pool
    m_threadPool.reset(new threadPool<httpParser>(8, 10000)); // 8 threadNum, 10000: maxRequestNum

    // socket
    m_listenFd = socket(AF_INET, SOCK_STREAM, 0);
    int flag = 1;
    setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    // bind
    m_serverAddress.sin_family = AF_INET;
    m_serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    m_serverAddress.sin_port = htons(SERVER_PORT);
    bind(m_listenFd, (struct sockaddr*) &m_serverAddress, sizeof(m_serverAddress));
    // listen
    listen(m_listenFd, 10); // 10 none use.
    // epoll create
    m_epollFd = epoll_create(EPOLL_MAX_NUM);
    if (m_epollFd < 0) {
        perror("epoll create");
        close(m_listenFd);
        return RetValue::FAIL;
    }

    m_event.events = EPOLLIN;
    m_event.data.fd = m_listenFd;
    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_listenFd, &m_event) < 0) {
        perror("epoll ctl add m_listenFd ");
        close(m_epollFd);
        close(m_listenFd);
        return RetValue::FAIL;
    }
    m_eventArr = (epoll_event*) malloc(sizeof(struct epoll_event) * EPOLL_MAX_NUM);

    m_parser.reset(new httpParser[MAX_USER_DATA_NUM]());

    // Init the timer
    timer::m_epollFd = m_epollFd;
    alarm(2); // 2:two second.
    addSig(SIGALRM, sig_handler);
    addSig(SIGTERM, sig_handler);
    
    // init the pipe.
    socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipe);
    u_pipe = m_pipe;
    setnonblocking(m_pipe[0]);
    
    epoll_event sigEvent;
    sigEvent.events = sigEvent.events | EPOLLIN;
    sigEvent.data.fd = m_pipe[0];
    epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_pipe[0], &sigEvent);
    m_logger->writeLog("start success", logClass::INFO);
    return RetValue::SUCCESS;
}


RetValue epollConnect::run()
{
    while (!m_stopServer) {
        int active_fds_cnt = epoll_wait(m_epollFd, m_eventArr, EPOLL_MAX_NUM, -1);
        for (m_epollIndex = 0; m_epollIndex < active_fds_cnt; m_epollIndex++) {
            if (m_eventArr[m_epollIndex].data.fd == m_listenFd) {
                if (acceptNewConnection() == RetValue::FAIL) {
                    return RetValue::FAIL;
                }
            } else if (m_eventArr[m_epollIndex].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                int client_fd = m_eventArr[m_epollIndex].data.fd;
                timer *correspondingTimer = m_userData[client_fd].curTimer;
                callBackFunction(&m_userData[client_fd]);
                if (correspondingTimer) {
                    m_lst.deleteTimer(correspondingTimer);
                }
            } else if (m_eventArr[m_epollIndex].events | EPOLLIN && m_eventArr[m_epollIndex].data.fd == m_pipe[0]) {
                dealSignal();
            } else if (m_eventArr[m_epollIndex].events & EPOLLIN) {
                if (readDataFromTriggerSocket() == RetValue::FAIL) {
                    return RetValue::FAIL;
                }
            } else if (m_eventArr[m_epollIndex].events & EPOLLOUT) {
                if (writeDataToTriggerSocket() == RetValue::FAIL) {
                    return RetValue::FAIL;
                }
            }
        }

        if (m_timeout) {
            m_timeout = false;
            m_lst.tick();
            alarm(2);
        }
    }
    close(m_epollFd);
    close(m_listenFd);
    delete []m_eventArr;
    return RetValue::SUCCESS;
}

epollConnect::~epollConnect()
{
    if (m_response != nullptr) {
        delete m_response;
        m_response = nullptr;
    }
}

epollConnect::epollConnect()
{
    
}

RetValue epollConnect::acceptNewConnection()
{
    int client_fd = accept(m_listenFd, (struct sockaddr*) &m_clientAddress, &m_clientLen);
    if (client_fd < 0) {
        perror("accept");
        return RetValue::FAIL;
    }
    char ip[20];
    m_logger->writeLog(logClass::INFO, "new connection[%s:%d]\n", inet_ntop(AF_INET, &m_clientAddress.sin_addr, ip, sizeof(ip)), ntohs(m_clientAddress.sin_port));
    m_event.events = EPOLLIN | EPOLLET;
    m_event.data.fd = client_fd;
    setnonblocking(client_fd);

    timer *newTimer = new timer();
    time_t curTime = time(NULL);
    m_userData[client_fd].socket = client_fd;
    m_userData[client_fd].curTimer = newTimer;

    newTimer->m_dealFun = callBackFunction;
    newTimer->m_expireTime = curTime + 3 * 5;
    newTimer->m_clientData = &m_userData[client_fd];
    m_lst.addTimer(newTimer);
    epoll_ctl(m_epollFd, EPOLL_CTL_ADD, client_fd, &m_event);
    return RetValue::SUCCESS;
}

RetValue epollConnect::readDataFromTriggerSocket()
{
    m_logger->writeLog("EPOLLIN", logClass::INFO);
    int client_fd = m_eventArr[m_epollIndex].data.fd;

    m_parser.get()[client_fd].setSocketFd(client_fd);
    m_parser.get()[client_fd].setEpollFd(m_epollFd);
    bool ret = m_parser.get()[client_fd].ReadFromSocket(client_fd, nullptr, 1024, 0, &m_event);
    m_logger->writeLog(logClass::INFO, "recvfinal:%s", m_parser.get()[client_fd].m_readBuff.get());
    if (ret == false) {
        m_logger->writeLog("err in read process", logClass::ERROR);
        timer *correspondingTimer = m_userData[client_fd].curTimer;
        callBackFunction(&m_userData[client_fd]);
        if (correspondingTimer) {
            m_lst.deleteTimer(correspondingTimer);
        }
        return RetValue::FAIL;
    } else {
        timer *correspondingTimer = m_userData[client_fd].curTimer;
        time_t curTime = time(NULL);
        if (correspondingTimer != NULL) {
            correspondingTimer->m_expireTime = curTime + 3 * 5;
            m_lst.adjustTimer(correspondingTimer);
        }

        m_threadPool->append(&m_parser.get()[client_fd]);
    }
    return RetValue::SUCCESS;
}

RetValue epollConnect::writeDataToTriggerSocket()
{
    m_logger->writeLog("EPOLLOUT", logClass::INFO);
    int client_fd = m_eventArr[m_epollIndex].data.fd;
    bool ret = m_parser.get()[client_fd].HttpResponse();
    if (ret == true) {
        timer *correspondingTimer = m_userData[client_fd].curTimer;
        time_t curTime = time(NULL);
        if (correspondingTimer != NULL) {
            correspondingTimer->m_expireTime = curTime + 3 * 5;
            m_lst.adjustTimer(correspondingTimer);
        }
    } else {
        m_logger->writeLog("need to close", logClass::ERROR);
        timer *correspondingTimer = m_userData[client_fd].curTimer;
        callBackFunction(&m_userData[client_fd]);
        if (correspondingTimer) {
            m_lst.deleteTimer(correspondingTimer);
        }
        return RetValue::FAIL;
    }
    return RetValue::SUCCESS;
}

void epollConnect::dealSignal()
{
    int num = recv(m_pipe[0], m_signals, sizeof(m_signals), 0);
    if (num > 0) {
        for (int index = 0; index < num; index++) {
            switch (m_signals[index]) {
                case SIGALRM:
                    m_timeout = true;
                    break;
                case SIGTERM:
                    m_stopServer = true;
                    break;
                default:
                    break;
            }
        }
    }
}
