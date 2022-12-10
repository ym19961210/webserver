#include <iostream>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include "test1.h"
#include "test3.h"

int test(int x1, int x2, const char* format, ...)
{
    printf(format);
    // printf("\n");
    va_list p;
    va_start(p, format);
    // int x = 0;
    // x = va_arg(p, int);
    // printf("%d", x);
    // x = va_arg(p, int);
    // printf("%d", x);
    char ar[100];
    vsnprintf(ar, 100, format, p);

    printf("%s", ar);
    va_end(p);

    char f[12];
    f[0] = 'f';
    std::string s(f);
    std::cout<<s<<std::endl;
}


class A
{
public:
explicit A(double d) : d(d) {}
explicit operator double() const { return d; } // Compliant
private:
double d;
};

int main(void)
{
A a{3.1415926535897932384626433832795028841971693993751058209749445923078};

double tmp1{a};
// float tmp2{a}; //compilation error instead of warning, prevents from data
// precision loss

return 0;
}
    
