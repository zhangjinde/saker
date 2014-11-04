#include "time.h"
#ifndef _WIN32
#include <sys/time.h>
#endif
#include "common/common.h"



long long ustime(void) {
    struct timeval tv;
    long long ust;

    gettimeofday(&tv, NULL);
    ust = ((long long)tv.tv_sec)*1000000;
    ust += tv.tv_usec;
    return ust;
}
