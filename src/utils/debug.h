#ifndef _DEBUG__H_
#define _DEBUG__H_

#include <signal.h>
#include "common/types.h"

#define ugAssert(_e) ((_e)?(void)0 : (_ugAssert(#_e,__FILE__,__LINE__),_exit(1)))
#define ugPanic(_e) _ugPanic(#_e,__FILE__,__LINE__),_exit(1)

#define ugAssertWithInfo(_c,_o,_e) ugAssert(_e)

void _ugAssert(char *estr, char *file, int line);

void _ugPanic(char *msg, char *file, int line);


#ifdef HAVE_BACKTRACE

void  sigsegv_handler(int sig, siginfo_t *info, void *secret);

#endif

#endif
