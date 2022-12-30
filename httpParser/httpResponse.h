#ifndef HTTP_RESPONSE
#define HTTP_RESPONSE

#include <stdio.h>

/**
 * @brief Add content for http response.
 *
 */
class httpResponser {
public:
    static const int WRITE_BUFFER_SIZE = 1024; /// write buffer size.

    /**
     * @brief Get the write buffer.
     *
     * @return char* Pointer of write buffer.
     */
    char* getWriteBuffer()
    {
        return m_write_buf;
    }

    /**
     * @brief Get the Write index(which have been writen).
     *
     * @return The index value.
     */
    int getWriteIndex()
    {
        return m_write_idx;
    }

    /**
     * @brief Set the Write index.
     *
     * @param setValue The value which is set.
     */
    void setWriteIndex(int setValue)
    {
        m_write_idx = setValue;
    }

    /**
     * @brief Add content to write buffer.
     *
     * @param format The content which is going to be added to response.
     * @param ... Variable parameters.
     * @retval true Add successfully.
     * @retval false Add unsuccessfully.
     */
    bool add_response(const char *format, ...);

    /**
     * @brief Add status line to the http response.
     *
     * @param status Http status code. eg 200 is ok.
     * @param title Status title.
     * @retval true Add successfully.
     * @retval false Add unsuccessfully.
     */
    bool add_status_line(int status, const char *title);

    /**
     * @brief Add headers to the http response.
     *
     * @param content_len Response Content length.
     * @retval true Add successfully.
     * @retval false Add unsuccessfully.
     */
    bool add_headers(int content_len);

    /**
     * @brief Add content length to the http response.
     *
     * @param content_len Response Content length.
     * @retval true Add successfully.
     * @retval false Add unsuccessfully.
     */
    bool add_content_length(int content_len);

    /**
     * @brief Add content type to the http response.
     *
     * @retval true Add successfully.
     * @retval false Add unsuccessfully.
     */
    bool add_content_type();

    /**
     * @brief Add long connection status to the http response.
     *
     * @retval true Add successfully.
     * @retval false Add unsuccessfully.
     */
    bool add_linger();

    /**
     * @brief Add blank line to the http response.
     *
     * @retval true Add successfully.
     * @retval false Add unsuccessfully.
     */
    bool add_blank_line();

    /**
     * @brief Add content to the http response.
     *
     * @param content Pointer which points to the content.
     * @retval true Add successfully.
     * @retval false Add unsuccessfully.
     */
    bool add_content(const char *content);

    /**
     * @brief Clear write buffer.
     *
     */
    void clearBuffer();

private:
    char m_write_buf[WRITE_BUFFER_SIZE]; /// write buffer.
    int m_write_idx = 0; /// write index.
};

#endif