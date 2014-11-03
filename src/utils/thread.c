#include "thread.h"


void xsleep(size_t ms) {
#ifdef OS_WIN
    Sleep(ms);
#elif OS_UNIX
    usleep(ms*1000);
#else
    //todo...
#endif
}

