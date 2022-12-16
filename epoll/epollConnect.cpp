#include "epollConnect.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include "timer/timerClass.h"
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

RetValue epollConnect::run()
{
    // init the logger
    m_logger = log::getInstance("/home/miyan/web/webserver/log/ym.txt");
    m_logger->init();
    
    // init Httpresponse
    bool timeout = false;
    bool stop_server = false;

    m_response = new httpResponser();

    //init thread pool
    m_threadPool.reset(new threadPool<httpParser>(8, 10000)); // 8 threadNum, 10000: maxRequestNum

    int listen_fd = 0;
    int client_fd = 0;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_len;

    struct epoll_event event, *my_events;
    my_events = nullptr;
    // socket
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int flag = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    // bind
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);
    bind(listen_fd, (struct sockaddr*) &server_addr, sizeof(server_addr));
    // listen
    listen(listen_fd, 10); // 10 none use.
    // epoll create
    m_epollFd = epoll_create(EPOLL_MAX_NUM);
    if (m_epollFd < 0) {
        perror("epoll create");
        close(listen_fd);
        return RetValue::FAIL;
    }

    event.events = EPOLLIN;
    event.data.fd = listen_fd;
    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, listen_fd, &event) < 0) {
        perror("epoll ctl add listen_fd ");
        close(m_epollFd);
        close(listen_fd);
        return RetValue::FAIL;
    }
    my_events = (epoll_event*) malloc(sizeof(struct epoll_event) * EPOLL_MAX_NUM);

    m_parser.reset(new httpParser[65535]());


    // Init the timer
    clientData userData[65535];
    sort_list_timer lst;
    timer::m_epollFd = m_epollFd;

    alarm(2); // 2:two second.
    addSig(SIGALRM, sig_handler);
    addSig(SIGTERM, sig_handler);
    int pipe[2]; // 2 a socket pair.
    socketpair(PF_UNIX, SOCK_STREAM, 0, pipe);
    u_pipe = pipe;
    setnonblocking(pipe[0]);
    char signals[1024]; // 1024:max signal num
    epoll_event sigEvent;
    sigEvent.events = sigEvent.events | EPOLLIN;
    sigEvent.data.fd = pipe[0];
    epoll_ctl(m_epollFd, EPOLL_CTL_ADD, pipe[0], &sigEvent);
    m_logger->writeLog("start success", logClass::INFO);
    while (!stop_server) {
        int active_fds_cnt = epoll_wait(m_epollFd, my_events, EPOLL_MAX_NUM, -1);
        int i = 0;
        for (i = 0; i < active_fds_cnt; i++) {
            if (my_events[i].data.fd == listen_fd) {
                // calculate(2, "");
                // printf("sc:%d listen\n", my_events[i].data.fd);
                // fflush(stdout);

                client_fd = accept(listen_fd, (struct sockaddr*) &client_addr, &client_len);
                if (client_fd < 0) {
                    perror("accept");
                    continue;
                }
                char ip[20];
                m_logger->writeLog(logClass::INFO, "new connection[%s:%d]\n", inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip)), ntohs(client_addr.sin_port));
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = client_fd;
                setnonblocking(client_fd);
                m_logger->writeLog(logClass::INFO, "new fd is %d", client_fd);

                timer *newTimer = new timer();
                time_t curTime = time(NULL);
                userData[client_fd].socket = client_fd;
                userData[client_fd].curTimer = newTimer;

                newTimer->m_dealFun = callBackFunction;
                newTimer->m_expireTime = curTime + 3 * 5;
                newTimer->m_clientData = &userData[client_fd];
                lst.addTimer(newTimer);
                epoll_ctl(m_epollFd, EPOLL_CTL_ADD, client_fd, &event);
                // calculate(0, "listen and accept after");
            } else if (my_events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                client_fd = my_events[i].data.fd;
                timer *correspondingTimer = userData[client_fd].curTimer;
                callBackFunction(&userData[client_fd]);
                if (correspondingTimer) {
                    lst.deleteTimer(correspondingTimer);
                }
            } else if (my_events[i].events | EPOLLIN && my_events[i].data.fd == pipe[0]) {
                int num = recv(pipe[0], signals, sizeof(signals), 0);
                if (num > 0) {
                    for (int i = 0; i < num; i++) {
                        switch (signals[i]) {
                            case SIGALRM:
                                timeout = true;
                                break;
                            case SIGTERM:
                                stop_server = true;
                                break;
                            default:
                                break;
                        }
                    }
                }
            } else if (my_events[i].events & EPOLLIN) {
                // calculate(2, "");
                // printf("sc:%d epoll in\n", my_events[i].data.fd);
                // fflush(stdout);
                m_logger->writeLog("EPOLLIN", logClass::INFO);
                client_fd = my_events[i].data.fd;

                m_parser.get()[client_fd].setSocketFd(client_fd);
                m_parser.get()[client_fd].setEpollFd(m_epollFd);
                bool ret = m_parser.get()[client_fd].ReadFromSocket(client_fd, nullptr, 1024, 0, &event);
                m_logger->writeLog(logClass::INFO, "recvfinal:%s", m_parser.get()[client_fd].m_readBuff.get());
                if (ret == false) {
                    m_logger->writeLog("err in read process", logClass::ERROR);
                    timer *correspondingTimer = userData[client_fd].curTimer;
                    callBackFunction(&userData[client_fd]);
                    if (correspondingTimer) {
                        lst.deleteTimer(correspondingTimer);
                    }
                } else {
                    timer *correspondingTimer = userData[client_fd].curTimer;
                    time_t curTime = time(NULL);
                    if (correspondingTimer != NULL) {
                        correspondingTimer->m_expireTime = curTime + 3 * 5;
                        lst.adjustTimer(correspondingTimer);
                    }

                    m_threadPool->append(&m_parser.get()[client_fd]);
                }
                // calculate(0, "read data from socket after");
            } else if (my_events[i].events & EPOLLOUT) {
                // calculate(2, "");
                // printf("sc:%d epoll out\n", my_events[i].data.fd);
                // fflush(stdout);

                m_logger->writeLog("EPOLLOUT", logClass::INFO);
                client_fd = my_events[i].data.fd;
                bool ret = m_parser.get()[client_fd].HttpResponse();
                if (ret == true) {
                timer *correspondingTimer = userData[client_fd].curTimer;
                    time_t curTime = time(NULL);
                    if (correspondingTimer != NULL) {
                        correspondingTimer->m_expireTime = curTime + 3 * 5;
                        lst.adjustTimer(correspondingTimer);
                    }
                } else {
                    m_logger->writeLog("need to close", logClass::ERROR);
                    timer *correspondingTimer = userData[client_fd].curTimer;
                    callBackFunction(&userData[client_fd]);
                    if (correspondingTimer) {
                        lst.deleteTimer(correspondingTimer);
                    }
                }
                // calculate(0, "write data to socket after");
                // printf("write finish\n");
                // fflush(stdout);
            }
        }

        if (timeout) {
            timeout = false;
            lst.tick();
            alarm(2);
        }
    }
    close(m_epollFd);
    close(listen_fd);
    delete []my_events;
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