#ifndef RETURNVALUE_DEF
#define RETURNVALUE_DEF

#include <stdint.h>

enum class RetValue : unsigned int {
    SUCCESS = 0,
    FAIL = 1,
};

enum class RetParserLine : uint8_t {
    LINE_COMPLETED = 0,
    LINE_UNCOMPLETED,
    BAD_LINE,
};

enum class RetParserState : uint8_t {
    NO_REQUEST = 0,
    GET_REQUEST,
    BAD_REQUEST,
    NO_RESOURCE,
    FORBIDDEN_REQUEST,
    FILE_REQUEST,
    INTERNAL_ERROR,
    CLOSED_CONNECTION
};

#endif