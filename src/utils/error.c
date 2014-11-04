#include "error.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "common/types.h"

int xerrno() {
    return errno;
}

const char *xerrstr(unsigned long errnum) {
    return strerror(errnum);
}

const char *xerrmsg() {
    return xerrstr(xerrno());
}

void  xerrclear() {
    errno = 0;
}


