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
#include <sys/socket.h>
#include "timer/timerClass.h"
#include <arpa/inet.h>

#define SERVER_PORT         (8001)
#define EPOLL_MAX_NUM       (2048)
#define BUFFER_MAX_LEN      (4096)

class epollConnect {
public:
    /**
     * @brief Construct a epollConnect object.
     *
     */
    epollConnect();

    /**
     * @brief Destroy the epoll Connect object
     *
     */
    ~epollConnect();

    /**
     * @brief This function is called to run the working thread.
     *
     * @retval RetValue::FAIL Some failures in working thread.
     * @retval RetValue::SUCCESS Exit successfully.
     */
    RetValue run();

    /**
     * @brief Init the variables and components.
     *
     * @retval RetValue::FAIL Some failures in initializer.
     * @retval RetValue::SUCCESS Init successfully.
     */
    RetValue init();

private:
    int m_byteHadRead = 0; ///< The byte numbers of content have been read one time.
    int m_byteHadReadTotal = 0; ///< The byte numbers of content have been read totally during one tcp connect.
    int m_epollFd = 0; ///< Fd of epoll.
    httpResponser *m_response = nullptr; ///< Doing repsonse action.
    std::unique_ptr<httpParser> m_parser; ///< Parsing the content send by client.
    std::unique_ptr<threadPool<httpParser> > m_threadPool; ///< Thread pool, once connection is occured, one worker thread will be waked and do some actions.
    log* m_logger; ///< logger handle.

    // listen Fd.
    int m_listenFd = 0; ///< Fd of listening.
    struct sockaddr_in m_serverAddress; /// Address of server.
    struct sockaddr_in m_clientAddress; /// Address of client.
    socklen_t m_clientLen = 0; /// Lenght of client Address.

    static constexpr int MAX_USER_DATA_NUM = 65535; /// max client data number.
    clientData m_userData[MAX_USER_DATA_NUM]; /// Array used to store client data.
    struct epoll_event m_event; /// epoll event for adding new event.

    /**
     * @brief Business function for accepting new connection.
     *
     * @retval RetValue::FAIL Some failures in the deal.
     * @retval RetValue::SUCCESS Accept successfully.
     */
    RetValue acceptNewConnection();

    /**
     * @brief Read data from the trigger socket.
     *
     * @retval RetValue::FAIL Some failures in the deal.
     * @retval RetValue::SUCCESS Read successfully.
     */
    RetValue readDataFromTriggerSocket();

    /**
     * @brief write data from the trigger socket.
     *
     * @retval RetValue::FAIL Some failures in the deal.
     * @retval RetValue::SUCCESS Write successfully.
     */
    RetValue writeDataToTriggerSocket();

    /**
     * @brief When signals come, this function will deal with them.
     *
     */
    void dealSignal();

    // timer sort list
    sort_list_timer m_lst; /// A link list used to sort the connection according to their time when communicating with the server last time.

    struct epoll_event *m_eventArr = nullptr; /// event array used in epoll_wait.
    int m_epollIndex = 0; /// the epoll event index of being awaked by system.

    // signal related.
    int m_pipe[2]; // 2 a socket pair.
    char m_signals[1024]; // 1024:max signal num


    bool m_timeout = false; /// flag used to identify whether there is timeout event occurs.
    bool m_stopServer = false; /// flag used to identify server should be stopped.

};

#endif