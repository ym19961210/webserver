// ofstream::open / ofstream::close
#include <fstream>      // std::ofstream
#include "log.h"
#include <unistd.h>
#include <string>
#include <memory>
#include <vector>

int main () {
    log *f = log::getInstance("/home/miyan/web/webserver/log/ym.txt");
    pthread_t tid = f->init();
    std::string logWrite = "ym test";
    std::vector<logClass> q{logClass::DEBUG, logClass::WARNING, logClass::INFO, logClass::ERROR};
    for (int i = 0; i < 200000; i++) {
      std::string newStr = logWrite + std::to_string(i); // The address of newStr will not change during every loop, and it will assign the address in arr with this address. Though all elements are the same.
      f->writeLog(newStr.c_str(), q[i % 4]);
    }
    pthread_join(tid, NULL);
    return 0;
}