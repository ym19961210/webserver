#ifndef HTTP_DEF
#define HTTP_DEF

#include <stdint.h>

/**
 * @brief Http method.
 *
 */
enum class HttpMethod : uint8_t {
    GET = 0,
    POST,
    HEAD,
    PUT,
    DELETE,
    TRACE,
    OPTIONS,
    CONNECT,
    PATH,
    UNKNOWN,
};

/**
 * @brief To be deprecated.
 *
 */
struct HttpMethodAndRelatedStr {
    HttpMethod method;
    const char *methodStr;
};

/**
 * @brief Parsing state.
 *
 */
enum class ParsingState : uint8_t {
    REQUESTLINE = 0,
    HEADER,
    CONTENT
};

#endif
