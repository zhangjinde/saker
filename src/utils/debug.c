#include "debug.h"
#include "utils/logger.h"

static void bugReportStart(void)
{
    LOG_WARNING("\n\n=== BUG REPORT START: Cut & paste starting from here ===");
}


void _ugAssert(char* estr, char* file, int line)
{
    bugReportStart();
    LOG_WARNING("=== ASSERTION FAILED ===");
    LOG_WARNING("==> %s:%d '%s' is not true",file,line,estr);
#ifdef HAVE_BACKTRACE
    LOG_WARNING("(forcing SIGSEGV in order to print the stack trace)");
#endif
    *((char*)-1) = 'x';
}

void _ugPanic(char* msg, char* file, int line)
{
    bugReportStart();
    LOG_WARNING("------------------------------------------------");
    LOG_WARNING("!!! Software Failure. Press left mouse button to continue");
    LOG_WARNING("Guru Meditation: %s #%s:%d",msg,file,line);
#ifdef HAVE_BACKTRACE
    LOG_WARNING("(forcing SIGSEGV in order to print the stack trace)");
#endif
    LOG_WARNING("------------------------------------------------");
    *((char*)-1) = 'x';
}




