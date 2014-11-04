#include "error.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "common/types.h"


int   xerrno() {
#if defined(OS_WIN)
    return GetLastError();
#else
    return errno;
#endif
}


const char *xerrstr(unsigned long errnum) {

#ifdef OS_WIN
    size_t      offset = 0;
    static char error_string[1024]= {0};
    LPVOID lpMsgBuf;

    offset += snprintf(error_string, sizeof(error_string), "[0x%08lX] ", errnum);

    if (0 == FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                errnum,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &lpMsgBuf,
                0,
                NULL)) {
        _snprintf(error_string + offset, sizeof(error_string) - offset, "unable to find message text [0x%08lX]", GetLastError());
        return error_string;
    }
    return (LPCTSTR)lpMsgBuf;
#else
    return strerror(errnum);
#endif
}



const char *xerrmsg() {
    return xerrstr(xerrno());
}


void  xerrclear() {
#if defined(OS_WIN)
    SetLastError(0);
#else
    errno = 0;
#endif

}


