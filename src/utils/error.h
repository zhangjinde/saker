#ifndef _ERROR__H_
#define _ERROR__H_

#include "common/types.h"
#include <errno.h>

int   xerrno(void);

const char *xerrstr(unsigned long errnum);

const char *xerrmsg(void);

void  xerrclear(void);

#endif
