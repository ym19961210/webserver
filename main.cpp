#include "epoll/epollConnect.h"
#include "httpParser/httpResponse.h"

int main()
{
    epollConnect * ptr = new epollConnect();
    log* logHandle = log::getInstance("/home/miyan/web/webserver/log/ym.txt");
    logHandle->init();


    if (ptr->init() != RetValue::SUCCESS)
    {
        logHandle->writeLog(logClass::ERROR, "init epoll fail");
        delete logHandle;
        logHandle = nullptr;
        delete ptr;
        ptr = nullptr;
        return -1;
    }

    ptr->run();
    delete ptr;
    return 0;
}