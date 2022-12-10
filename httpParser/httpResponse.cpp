#include "httpResponse.h"
#include <iostream>
#include <stdarg.h>
#include <string.h>
#include "log/log.h"

bool httpResponser::add_response(const char *format, ...)
{
    if (m_write_idx >= WRITE_BUFFER_SIZE)
        return false;
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list);
    if (len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx))
    {
        va_end(arg_list);
        return false;
    }
    m_write_idx += len;
    va_end(arg_list);
    // log* lger = log::getInstance("/home/miyan/web/webserver/log/ym.txt");
    // lger->writeLog(logClass::INFO, "request:%s", m_write_buf);

    return true;
}

bool httpResponser::add_status_line(int status, const char *title)
{
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}
bool httpResponser::add_headers(int content_len)
{
    return add_content_length(content_len) && add_linger() &&
           add_blank_line();
}
bool httpResponser::add_content_length(int content_len)
{
    return add_response("Content-Length:%d\r\n", content_len);
}
bool httpResponser::add_content_type()
{
    return add_response("Content-Type:%s\r\n", "text/html");
}
bool httpResponser::add_linger()
{
    return add_response("Connection:%s\r\n", "keep-alive");
}
bool httpResponser::add_blank_line()
{
    return add_response("%s", "\r\n");
}
bool httpResponser::add_content(const char *content)
{
    return add_response("%s", content);
}

void httpResponser::clearBuffer()
{
    memset(m_write_buf, 0, WRITE_BUFFER_SIZE);
}

