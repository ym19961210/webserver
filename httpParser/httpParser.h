#ifndef HTTP_PARSER
#define HTTP_PARSER

#include "public_def/returnValue.h"
#include "public_def/httpDef.h"
#include <memory>
#include <sys/stat.h>
#include <unistd.h>
#include "httpResponse.h"
#include <sys/uio.h>
#include <iostream>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "log/log.h"

/**
 * @brief Parse the http content recevied from sockets.
 * 
 */
class httpParser {
public:
    /**
     * @brief Construct a new http Parser object.
     *
     */
    httpParser();

    /**
     * @brief Construct a new http Parser object with assigned parameters.
     *
     * @param[in] buffer A empty buffer for saving reading information.
     * @param[in] bufferLen buffer lenght.
     * @param[in] epollFd Epoll fd.
     */
    httpParser(std::unique_ptr<char> buffer, int bufferLen, int epollFd)
    {
        if (buffer != nullptr) {
            m_readBuff = std::move(buffer);
        } else {
            m_readBuff.reset(new char[1024]);
        }
        
        m_totalIndex = bufferLen;

        reponser.reset(new httpResponser());
        m_epollfd = epollFd;
        init(-1, -1);
    }

    /**
     * @brief Destroy the http Parser object
     *
     */
    ~httpParser();

    /**
     * @brief A internal function which helps to parse one line content.
     *
     * @retval RetParserLine::LINE_UNCOMPLETED This line is uncompleted.
     * @retval RetParserLine::LINE_COMPLETED This line is completed.
     * @retval RetParserLine::BAD_LINE This line has bad content.
     */
    RetParserLine parserOneLine();

    /**
     * @brief Parse the http request line.
     *
     * @param[in] text The pointer which points to the start of content which is going to be parsed.
     * @retval RetParserState::BAD_REQUEST There is error in the request.
     * @retval RetParserState::NO_REQUEST Need more parsing in next process.
     */
    RetParserState parserRequestLine(char *text);

    /**
     * @brief Parse the http request header part.
     *
     * @param[in] text The pointer which points to the start of content which is going to be parsed.
     * @retval RetParserState::BAD_REQUEST There is error in the request.
     * @retval RetParserState::NO_REQUEST Need more parsing in next process.
     * @retval RetParserState::GET_REQUEST Request type is GET.
     */
    RetParserState parse_headers(char *text);

    /**
     * @brief Parse the http request content part.
     *
     * @param[in] text The pointer which points to the start of content which is going to be parsed.
     * @retval RetParserState::NO_REQUEST Need more parsing in next process.
     * @retval RetParserState::GET_REQUEST Request type is GET.
     */
    RetParserState parse_content(char *text);

    /**
     * @brief Parser state machine which is going to excute the parser function in order.
     *
     * @retval RetParserState::BAD_REQUEST There is error in the request.
     * @retval RetParserState::NO_REQUEST Need more parsing in next process.
     * @retval RetParserState::GET_REQUEST Request type is GET.
     * @retval RetParserState::INTERNAL_ERROR Internal error.
     */
    RetParserState ParserStateMachine();

    /**
     * @brief Process the request content, judge the status of the file which is requested.
     *
     * @retval RetParserState::NO_RESOURCE There is no resource which users request.
     * @retval RetParserState::FORBIDDEN_REQUEST The file request is forbidden.
     * @retval RetParserState::BAD_REQUEST Request is bad.
     * @retval RetParserState::FILE_REQUEST The file request is OK.
     */
    RetParserState ProcessRequest();

    /**
     * @brief Modify the epoll fd status.
     *
     * @param epollFd The epoll fd.
     * @param fd The fd which is going to be changed status.
     * @param modEvent Epoll events which is set.
     */
    void modfd(int epollFd, int fd, uint32_t modEvent);

    /**
     * @brief Unmap the file which has been mapped into memory.
     *
     */
    void unmap();

    /**
     * @brief Do response to http request, write data to socket of client.
     *
     * @return true Write ok.
     * @return false Some error happens during process.
     */
    bool HttpResponse();

    /**
     * @brief Init the whole parser. If epollFd is set to -1, the internal fd of epoll will not be inited.
     *
     * @param[in] epollFd The epoll fd which is used to listen the epoll event.
     * @param[in] fd The socket which is used to write data to client.
     */
    void init(int epollFd, int fd);

    /**
     * @brief Get the Parsing state.
     *
     * @return Parsing state.
     */
    ParsingState GetParsingState()
    {
        return m_check_state;
    }

    /**
     * @brief Get the current line which has been parsed.
     *
     * @return The pointer of current line.
     */
    char *getCurrentLine()
    {
        return  m_readBuff.get() + m_currentLine;
    }
public:
    int m_processIndex = 0; /// The index of content which has been parsed.
    int m_totalIndex; /// Index of total content which is going to be parsed.
    std::unique_ptr<char> m_readBuff; /// Buffer saving the read content.

    // variables for requestLine
    char *m_url; /// Url of resources.
    HttpMethod m_method = HttpMethod::UNKNOWN; /// Http method.
    char *m_version; /// version value of http request. eg:HTTP1.0

    // check state
    ParsingState m_check_state; /// State of parsing machine.

    // variables for headerLine.
    bool m_keepAlive = false; /// Flag identifing whether the connection is long.
    int m_content_length = 0; /// length of http request content.
    char *m_host; /// host ip.

    // variables for Content.
    char *m_content; /// content pointer.
private:
    int m_currentLine; /// index of content which has been parsed.

    // variables for response.
    struct stat m_fileStatus; /// status of the file requested.
    char * m_fileAddress; /// address of file requested.
    std::unique_ptr<httpResponser> reponser; /// used to do response of http request.
    struct iovec m_iv[2]; /// vector used to send data.
    int m_iv_count; /// count of vector which is going to be sent.
    int m_sendBytes; /// Bytes need to be sent.
    int m_bytesHaveSend; /// Bytes have to be sent.
    char m_urlFile[1024]; /// file url.

    // tmp for epollout
    int m_epollfd = 0; /// Epoll fd.
    int m_socket = 0; /// Socket used to send/receive data.

private:
    int m_byteHadRead = 0; /// Bytes which have been read one time.
    int m_byteHadReadTotal = 0; /// Bytes which have been read totally.
    log* m_logger;

public:
    /**
     * @brief Read information from socket.
     *
     * @param sockfd The socket read from.
     * @param buf To be deprecated.
     * @param len The max bytes which is read totally.
     * @param flags Flags used in function 'recv'.
     * @param event To be deprecated.
     * @return true Deal successfully.
     * @return false If return false, the caller should close connection.
     */
    bool ReadFromSocket(int sockfd, char *buf, size_t len, int flags, epoll_event *event);

    /**
     * @brief Set the Epoll Fd.
     *
     * @param epollFd The epoll fd which is going to be set.
     */
    void setEpollFd(int epollFd)
    {
        m_epollfd = epollFd;
    }

    /**
     * @brief Set the socket Fd.
     *
     * @param epollFd The socket fd which is going to be set.
     */
    void setSocketFd(int socket)
    {
        m_socket = socket;
    }

    RetParserState Process();

    /**
     * @brief Add content which is going to be sent according to the state.
     *
     * @param[in] ret A state of previous process, add response according to this state.
     * @return true Add content of responses ok.
     * @return false Some error during adding response content.
     */
    bool processWrite(RetParserState ret);

    /**
     * @brief Helper function of above function.
     *
     * @param[in] ret A state of previous process, add response according to this state.
     */
    void processWriteHelper(bool ret);
};

#endif