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

struct HttpRequestFrame
{
    char *url;
    HttpMethod method;
    char *version;
};

class httpParser {
public:
    httpParser();
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
    ~httpParser();
    RetParserLine parserOneLine();
    RetParserState parserRequestLine(char *text);
    RetParserState parse_headers(char *text);
    RetParserState parse_content(char *text);
    RetParserState ParserStateMachine();
    RetParserState ProcessRequest();
    void modfd(int epollFd, int fd, uint32_t modEvent);
    void unmap();
    bool HttpResponse();
    void init(int epollFd, int fd);
    ParsingState GetParsingState()
    {
        return m_check_state;
    }
    char *getCurrentLine()
    {
        return  m_readBuff.get() + m_currentLine;
    }
public:
    int m_processIndex = 0;
    int m_totalIndex;
    std::unique_ptr<char> m_readBuff;

    // variables for requestLine
    char *m_url;
    HttpMethod m_method = HttpMethod::UNKNOWN;
    char *m_version;

    // check state
    ParsingState m_check_state;

    // variables for headerLine.
    bool m_keepAlive = false;
    int m_content_length = 0;
    char *m_host;

    // variables for Content.
    char *m_content;
private:
    int m_currentLine;

    // variables for response.
    struct stat m_fileStatus;
    char * m_fileAddress;
    std::unique_ptr<httpResponser> reponser;
    struct iovec m_iv[2];
    int m_iv_count;
    int m_sendBytes;
    int m_bytesHaveSend;
    char m_urlFile[1024];

    // tmp for epollout
    int m_epollfd = 0;
    int m_socket = 0;

private:
    int m_byteHadRead = 0;
    int m_byteHadReadTotal = 0;

public:
    bool ReadFromSocket(int sockfd, char *buf, size_t len, int flags, epoll_event *event);

    void setEpollFd(int epollFd)
    {
        m_epollfd = epollFd;
    }

    void setSocketFd(int socket)
    {
        m_socket = socket;
    }

    RetParserState Process();
};

#endif