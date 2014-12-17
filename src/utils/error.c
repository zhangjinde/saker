#include "error.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "common/types.h"

int xerrno(void) {
    return errno;
}

const char *xerrstr(unsigned long errnum) {
    return strerror(errnum);
}

const char *xerrmsg(void) {
    return xerrstr(xerrno());
}

void  xerrclear(void) {
    errno = 0;
}


