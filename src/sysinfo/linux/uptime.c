#include "sysinfo/sysinfo.h"
#include <sys/sysinfo.h>


int SYSTEM_UPTIME(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    struct sysinfo  info;

    if (0 != sysinfo(&info))
        return UGERR;

    SET_UI64_RESULT(result, info.uptime);

    return UGOK;
}
