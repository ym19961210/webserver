#include "test.h"

extern void calculate(int flag, const char *message)
{
    tm = clock();
    if (flag == 0) {
        printf("now is %d\n", tm);
        if (message != NULL) {
            printf("excute %s, diff time is %d\n", message, tm - tmLastValue);
        }
        tmLastValue = tm;
    } else if (flag == 1) {
        printf("now is %d\n", tm);
        printf("I am %s\n", message);
    } else if (flag == 2) {
        tmLastValue = tm;
    }

    fflush(stdout);

}
