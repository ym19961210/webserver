#include "epoll/epollConnect.h"
#include "httpParser/httpResponse.h"

int main()
{
    epollConnect * ptr = new epollConnect();
    ptr->run();
    delete ptr;
    return 0;
}