#include "sysinfo/sysinfo.h"
#include "utils/string.h"


static int  get_cpu_num()
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return (int)sysInfo.dwNumberOfProcessors;
}


int SYSTEM_CPU_NUM_ONLINE(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    SET_UI64_RESULT(result, get_cpu_num());
    return UGOK;
}

/* windows we make max = online  */
int SYSTEM_CPU_NUM_MAX(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    SET_UI64_RESULT(result, get_cpu_num());
    return UGOK;
}


int SYSTEM_CPU_LOAD(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    SET_MSG_RESULT(result,xstrdup("not implemented"));
    return UGERR;
}


int SYSTEM_CPU_UTIL(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    SET_MSG_RESULT(result,xstrdup("not implemented"));
    return UGERR;
}

