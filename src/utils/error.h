#ifndef _ERROR__H_
#define _ERROR__H_

#include "common/types.h"
#include <errno.h>

int   xerrno() ;

const char* xerrstr(unsigned long errnum);

const char* xerrmsg();

void  xerrclear();

#endif
