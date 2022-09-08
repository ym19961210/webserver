#include <signal.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>


int *u_pipe;

void sig_handler(int sig)
{
    std::cout<<"ym num:"<<sig<<std::endl;
    if (sig == 14) {
        int msg = sig;
        send(u_pipe[1], (char *)(&msg), 1, 0);
        alarm(2);
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

void addsig(int sig, void(handler)(int), bool restart = true)
 {
     //创建sigaction结构体变量
    struct sigaction sa;
     memset(&sa, '\0', sizeof(sa));
 
     //信号处理函数中仅仅发送信号值，不做对应逻辑处理
     sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    //将所有信号添加到信号集中
    sigfillset(&sa.sa_mask);

    //执行sigaction函数
    assert(sigaction(sig, &sa, NULL) != -1);
}



int setFdNoblocking(int fd)
{
    int oldSet = fcntl(fd, F_GETFD);
    int newSet = oldSet | O_NONBLOCK;
    fcntl(fd, F_SETFD, newSet);
    return oldSet;
}

int main()
{
    alarm(2);
    addSig(SIGALRM, sig_handler);
    addSig(SIGTERM, sig_handler);

// addsig(SIGALRM, sig_handler, false);
// addsig(SIGTERM, sig_handler, false);
    int pipe[2];
    socketpair(PF_UNIX, SOCK_STREAM, 0, pipe);
    u_pipe = pipe;
    setFdNoblocking(pipe[0]);
    char signals[1024];
    int epollFd = epoll_create(5);
    epoll_event event;
    event.events = event.events | EPOLLIN;
    event.data.fd = pipe[0];

    epoll_ctl(epollFd, EPOLL_CTL_ADD, pipe[0], &event);    
    epoll_event ev[1024];
    while(1)
    {

        int ret = epoll_wait(epollFd, ev, 1024, -1);

        for (int i = 0; i < ret; i++) {
            if (ev[i].events | EPOLLIN && ev[i].data.fd == pipe[0]) {
                int num = recv(pipe[0], signals, sizeof(signals), 0);
                if (num > 0) {
                    for (int i = 0; i < num; i++) {
                        std::cout<<"liangryuyun"<<(int)(signals[i])<<std::endl;
                    }
                }
            }
        }

        // std::cout<<"i am in"<<std::endl;
    }
}