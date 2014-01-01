#include "sysinfo/sysinfo.h"
#include <time.h>
#include "utils/string.h"

int SYSTEM_BOOTTIME(const char* cmd, int argc, const char** argv, SYSINFO_RESULT* result)
{
    static int      corr = 0;
    LARGE_INTEGER   tickPerSecond, tick;
    static int  boottime = 0;
    BOOL        rc = FALSE;
    int     ret = UGERR;

    if (boottime == 0) {
        if (TRUE == (rc = QueryPerformanceFrequency(&tickPerSecond))) {
            if (TRUE == (rc = QueryPerformanceCounter(&tick))) {
                tick.QuadPart = tick.QuadPart / tickPerSecond.QuadPart;
                boottime = (int)(time(NULL) - tick.QuadPart);
                ret = UGOK;
            }
        }
    }
    SET_UI64_RESULT(result, boottime);
    return ret;
}

