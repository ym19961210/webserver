#include "httpParser.h"
#include "public_def/marcoMethod.h"
#include <iostream>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <fcntl.h>

//定义http响应的一些状态信息
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";

RetParserLine httpParser::parserOneLine()
{
    char curChar;
    for (; m_processIndex < m_totalIndex; m_processIndex++) {
        curChar = m_readBuff.get()[m_processIndex];

        if (curChar == '\r') {
            if (m_processIndex + 1 == m_totalIndex) {
                return RetParserLine::LINE_UNCOMPLETED;
            }
            if (m_readBuff.get()[m_processIndex + 1] == '\n') {
                m_readBuff.get()[m_processIndex++] = '\0';
                m_readBuff.get()[m_processIndex++] = '\0';
                return RetParserLine::LINE_COMPLETED;
            }
            return RetParserLine::BAD_LINE;
        } else if (curChar == '\n') {
            if (m_processIndex > 1 && m_readBuff.get()[m_processIndex - 1] == '\r') {
                m_readBuff.get()[m_processIndex - 1] = '\0';
                m_readBuff.get()[m_processIndex++] = '\0';
                return RetParserLine::LINE_COMPLETED;
            }
            return RetParserLine::BAD_LINE;
        }
    }
    return RetParserLine::LINE_UNCOMPLETED;
}

httpParser::httpParser()
{
    m_readBuff.reset(new char[1024]);
    m_totalIndex = 1024;

    reponser.reset(new httpResponser());
    
    init(-1, -1);

    m_logger = log::getInstance("/home/miyan/web/webserver/log/ym.txt");
    m_logger->init();
}

httpParser::~httpParser()
{
    unmap();
}

RetParserState httpParser::parserRequestLine(char *text)
{
    m_url = strpbrk(text, " \t");

    if (m_url == NULL) {
        return RetParserState::BAD_REQUEST;
    }

    *m_url++ = '\0'; // priority of operate * is greater than operator ++.

    m_version = text;
    const HttpMethodAndRelatedStr methodList[] = {
        {HttpMethod::GET, "GET"},
        {HttpMethod::POST, "POST"},
    };
    const uint16_t methodListSize = U16_ITEM_OF(methodList);
    for (uint16_t i = 0; i < methodListSize; i++) {
        if (strcasecmp(text, methodList[i].methodStr) == 0) {
            m_method = methodList[i].method;
        }
    }
    m_url += strspn(m_url, " \t"); // todo: if all space or \t?

    m_version = strpbrk(m_url, " \t");
    *m_version++ = '\0';
    m_version += strspn(m_version, " \t");

    if (strcasecmp(m_version, "HTTP/1.1") != 0) {
        return RetParserState::BAD_REQUEST;
    }

    // check url
    if (strncasecmp(m_url, "http://", 7) == 0) {
        m_url += 7;
        m_url = strchr(m_url, '/');
    }

    if (strncasecmp(m_url, "https://", 8) == 0) {
        m_url += 8;
        m_url = strchr(m_url, '/');
    }

    if (!m_url || m_url[0] != '/')
        return RetParserState::BAD_REQUEST;

    if (strlen(m_url) == 1)
        strcat(m_url, "welcome.html");
    m_check_state = ParsingState::HEADER;
    return RetParserState::NO_REQUEST;
}

RetParserState httpParser::parse_headers(char *text)
{
    if (text[0] == '\0')
    {
        if (m_content_length != 0)
        {
            m_check_state = ParsingState::CONTENT;
            return RetParserState::NO_REQUEST;
        }
        return RetParserState::GET_REQUEST;
    }
    else if (strncasecmp(text, "Connection:", 11) == 0)
    {
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0)
        {
            m_keepAlive = true;
        }
    }
    else if (strncasecmp(text, "Content-length:", 15) == 0)
    {
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atol(text);
    }
    else if (strncasecmp(text, "Host:", 5) == 0)
    {
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
    }
    else
    {
        m_logger->writeLog(logClass::INFO, "clt fd close：%s", text);
    }
    return RetParserState::NO_REQUEST;
}

RetParserState httpParser::parse_content(char *text)
{
    if (m_totalIndex >= (m_content_length + m_processIndex))
    {
        text[m_content_length] = '\0';
        m_content = text;
        return RetParserState::GET_REQUEST;
    }
    return RetParserState::NO_REQUEST;
}

RetParserState httpParser::Process()
{
    RetParserLine line_status = RetParserLine::LINE_COMPLETED;
    RetParserState ret = RetParserState::NO_REQUEST;
    char *text = 0;

    while ((m_check_state == ParsingState::CONTENT && line_status == RetParserLine::LINE_COMPLETED) ||
          ((line_status = parserOneLine()) == RetParserLine::LINE_COMPLETED))
    {
        text = getCurrentLine();
        m_currentLine = m_processIndex;
        switch (m_check_state)
        {
        case ParsingState::REQUESTLINE:
        {
            ret = parserRequestLine(text);
            if (ret == RetParserState::BAD_REQUEST)
                return ret;
            break;
        }
        case ParsingState::HEADER:
        {
            ret = parse_headers(text);
            if (ret == RetParserState::BAD_REQUEST)
                return ret;
            else if (ret == RetParserState::GET_REQUEST)
            {
                return ProcessRequest();
            }
            break;
        }
        case ParsingState::CONTENT:
        {
            ret = parse_content(text);
            if (ret == RetParserState::GET_REQUEST)
                return ProcessRequest();
            line_status = RetParserLine::LINE_UNCOMPLETED;
            break;
        }
        default:
            return RetParserState::INTERNAL_ERROR;
        }
    }
    
    return RetParserState::NO_REQUEST;
}

RetParserState httpParser::ParserStateMachine()
{
    RetParserLine line_status = RetParserLine::LINE_COMPLETED;
    RetParserState ret = RetParserState::NO_REQUEST;
    char *text = 0;

    while ((m_check_state == ParsingState::CONTENT && line_status == RetParserLine::LINE_COMPLETED) ||
          ((line_status = parserOneLine()) == RetParserLine::LINE_COMPLETED))
    {
        text = getCurrentLine();
        m_currentLine = m_processIndex;
        switch (m_check_state)
        {
        case ParsingState::REQUESTLINE:
        {
            ret = parserRequestLine(text);
            if (ret == RetParserState::BAD_REQUEST)
                return ret;
            break;
        }
        case ParsingState::HEADER:
        {
            ret = parse_headers(text);
            if (ret == RetParserState::BAD_REQUEST)
                return ret;
            else if (ret == RetParserState::GET_REQUEST)
            {
                return ProcessRequest();
            }
            break;
        }
        case ParsingState::CONTENT:
        {
            ret = parse_content(text);
            if (ret == RetParserState::GET_REQUEST)
                return ProcessRequest();
            line_status = RetParserLine::LINE_UNCOMPLETED;
            break;
        }
        default:
            return RetParserState::INTERNAL_ERROR;
        }
    }
    
    return RetParserState::NO_REQUEST;
}

RetParserState httpParser::ProcessRequest()
{
    const char * rootIndex = "/home/miyan/web/webserver/static_file";
    strcpy(m_urlFile, rootIndex);
    strcat(m_urlFile, m_url);

    if (stat(m_urlFile, &m_fileStatus) < 0)
    {
        return RetParserState::NO_RESOURCE;
    }


    if (!(m_fileStatus.st_mode & S_IROTH))
        return RetParserState::FORBIDDEN_REQUEST;

    if (S_ISDIR(m_fileStatus.st_mode))
    {
        return RetParserState::BAD_REQUEST;
    }


    int fd = open(m_urlFile, O_RDONLY);
    m_fileAddress = (char *)mmap(0, m_fileStatus.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    return RetParserState::FILE_REQUEST;
}

bool httpParser::processWrite(RetParserState ret)
{
    switch (ret)
    {
    case RetParserState::INTERNAL_ERROR:
    {
        reponser->add_status_line(500, error_500_title);
        reponser->add_headers(strlen(error_500_form));
        if (!reponser->add_content(error_500_form))
            return false;
        break;
    }
    case RetParserState::BAD_REQUEST:
    {
        reponser->add_status_line(404, error_404_title);
        reponser->add_headers(strlen(error_404_form));
        if (!reponser->add_content(error_404_form))
            return false;
        break;
    }
    case RetParserState::FORBIDDEN_REQUEST:
    {
        reponser->add_status_line(403, error_403_title);
        reponser->add_headers(strlen(error_403_form));
        if (!reponser->add_content(error_403_form))
            return false;
        break;
    }
    case RetParserState::FILE_REQUEST:
    {
        reponser->add_status_line(200, ok_200_title);
        if (m_fileStatus.st_size != 0)
        {
            reponser->add_headers(m_fileStatus.st_size);
            m_iv[0].iov_base = reponser->getWriteBuffer();
            m_iv[0].iov_len = reponser->getWriteIndex();
            m_iv[1].iov_base = m_fileAddress;
            m_iv[1].iov_len = m_fileStatus.st_size;
            m_iv_count = 2;
            m_sendBytes = reponser->getWriteIndex() + m_fileStatus.st_size;
            return true;
        }
        else
        {
            const char *ok_string = "<html><body></body></html>";
            reponser->add_headers(strlen(ok_string));
            if (!reponser->add_content(ok_string))
                return false;
        }
    }
    default:
        return false;
    }
    m_iv[0].iov_base = reponser->getWriteBuffer();
    m_iv[0].iov_len = reponser->getWriteIndex();
    printf("send:%s", reponser->getWriteBuffer());
    m_iv_count = 1;
    m_sendBytes = reponser->getWriteIndex();
    return true;
}

void httpParser::processWriteHelper(bool ret)
{
    if (ret == true) {
        modfd(m_epollfd, m_socket, EPOLLOUT);
    } else {
        epoll_ctl(m_epollfd, EPOLL_CTL_DEL, m_socket, 0);
        close(m_socket);
    }
}


bool httpParser::HttpResponse()
{
    while (1)
    {
        int temp = writev(m_socket, m_iv, m_iv_count);
        m_logger->writeLog(logClass::INFO, "send:%s", m_iv[0].iov_base);
        if (temp < 0)
        {
            if (errno == EAGAIN)
            {
                modfd(m_epollfd, m_socket, EPOLLOUT);
                return true;
            }
            unmap();
            return false;
        }

        m_bytesHaveSend += temp;
        m_sendBytes -= temp;
        if (m_bytesHaveSend >= m_iv[0].iov_len)
        {
            m_iv[0].iov_len = 0;
            m_iv[1].iov_base = m_fileAddress + (m_bytesHaveSend - reponser->getWriteIndex());
            m_iv[1].iov_len = m_sendBytes;
        }
        else
        {
            m_iv[0].iov_base = reponser->getWriteBuffer() + m_bytesHaveSend;
            m_iv[0].iov_len = m_iv[0].iov_len - m_bytesHaveSend;
        }

        if (m_sendBytes <= 0)
        {
            unmap();
            modfd(m_epollfd, m_socket, EPOLLIN);

            if (m_keepAlive)
            {
                init(-1, -1);
                return true;
            }
            else
            {
                init(-1, -1);
                return false;
            }
        }
    }
}

void httpParser::unmap()
{
    if (m_fileAddress)
    {
        munmap(m_fileAddress, m_fileStatus.st_size);
        m_fileAddress = 0;
    }
}

void httpParser::modfd(int epollFd, int fd, uint32_t modEvent)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = modEvent;
    epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &event);
}


void httpParser::init(int epollFd, int fd)
{
    if (epollFd != -1) {
        m_epollfd = epollFd;
        m_socket = fd;
    }
    m_check_state = ParsingState::REQUESTLINE;
    m_currentLine = 0;
    m_processIndex = 0;
    memset(reponser.get()->getWriteBuffer(), '\0', 1024);
    reponser.get()->setWriteIndex(0);
    
    memset(m_urlFile, '\0', 1024);
    memset(m_readBuff.get(), '\0', 1024);
    m_sendBytes = 0;
    m_bytesHaveSend = 0;
}

bool httpParser::ReadFromSocket(int sockfd, char *buf, size_t len, int flags, epoll_event *event)
{
    m_byteHadRead = 0;
    m_byteHadReadTotal = 0;
    while (true) {
        m_byteHadRead = recv(sockfd, m_readBuff.get() + m_byteHadRead, len - m_byteHadRead, flags);
        // m_logger->writeLog(logClass::INFO, "recv:%s", m_readBuff.get());
        if (m_byteHadRead < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                m_logger->writeLog("finish read", logClass::INFO);
                break;
            }
            m_logger->writeLog("clt fd close", logClass::INFO);
            return false;
        } else if (m_byteHadRead == 0) {
            m_logger->writeLog("close fd", logClass::INFO);
            epoll_ctl(m_epollfd, EPOLL_CTL_DEL, sockfd, event);
            close(sockfd);
            return false;
        } else {
            m_readBuff.get()[m_byteHadRead] = '\0';
            m_byteHadReadTotal += m_byteHadRead;
            continue;
        }
        return true;
    }
    return true;
}