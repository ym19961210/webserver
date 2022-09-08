#include "gtest/gtest.h"

#include "httpParser.h"
#include <string.h>
#include <iostream>
#include <memory>

TEST(test_httpParser, test0)
{
    std::unique_ptr<char> tempPointer(new char[8]);
    const char * testStr = "ainio\r\n";
    strcpy(tempPointer.get(), testStr);
    int len = strlen(tempPointer.get()) + 1;
    httpParser * ptr = new httpParser(std::move(tempPointer), len);
    RetParserLine ret = ptr->parserOneLine();
    EXPECT_EQ(ret, RetParserLine::LINE_COMPLETED);
    delete ptr;
}

TEST(test_httpParser, parserRequestLine)
{
    httpParser * ptr = new httpParser();
    const char * testStr = "GET / HTTP/1.1";
    char * p = new char[strlen(testStr) + 1];
    strcpy(p, testStr);

    RetParserState ret = ptr->parserRequestLine(p);
    
    EXPECT_EQ(ret, RetParserState::NO_REQUEST);
    EXPECT_STREQ(ptr->m_url, "/welcome.html");
    delete p;
    delete ptr;
}

TEST(test_httpParser, parse_headers)
{
    httpParser * ptr = new httpParser();
    const char * myStr = "Host: 114.132.152.53:7778";
    std::unique_ptr<char> p(new char[strlen(myStr) + 1]);
    strcpy(p.get(), myStr);

    (void)(ptr->parse_headers(p.get()));

    EXPECT_STREQ(ptr->m_host, "114.132.152.53:7778");

    delete ptr;
}

TEST(test_httpParser, ParserStateMachine)
{
    std::unique_ptr<char> tempPointer(new char[600]);
    const char * testStr = "GET / HTTP/1.1\r\nHost: 114.132.152.53:7778\r\nConnection: keep-alive\r\nCache-Control: max-age=0\r\nUpgrade-Insecure-Requests: 1\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/103.0.0.0 Safari/537.36\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: zh-CN,zh;q=0.9,en-US;q=0.8,en;q=0.7\r\nCookie: csrftoken=gIX8GrD4fFLvf5OuT4ZYEE9pWunKX2BXU0JBxGm0MVrOePu7q65qK9G0uoVEdB0I\r\n\r\n";
    strcpy(tempPointer.get(), testStr);
    int len = strlen(tempPointer.get()) + 1;
    httpParser * ptr = new httpParser(std::move(tempPointer), len);
    RetParserState ret = ptr->ParserStateMachine();
    EXPECT_EQ(ret, RetParserState::FILE_REQUEST);
    delete ptr;
}
