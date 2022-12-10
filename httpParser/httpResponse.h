#ifndef HTTP_RESPONSE
#define HTTP_RESPONSE

#include <stdio.h>

class httpResponser {
public:
    static const int WRITE_BUFFER_SIZE = 1024;
    char* getWriteBuffer()
    {
        return m_write_buf;
    }

    int getWriteIndex()
    {
        return m_write_idx;
    }

    void setWriteIndex(int setValue)
    {
        m_write_idx = setValue;
    }

    bool add_response(const char *format, ...);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_len);
    bool add_content_length(int content_len);
    bool add_content_type();
    bool add_linger();
    bool add_blank_line();
    bool add_content(const char *content);
    void clearBuffer();

private:
    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx = 0;
};

#endif