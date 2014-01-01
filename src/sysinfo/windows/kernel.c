#include "sysinfo/sysinfo.h"
#include "utils/string.h"

int KERNEL_MAXFILES(const char* cmd, int argc, const char** argv, SYSINFO_RESULT* result)
{
    SET_MSG_RESULT(result, xstrdup("not implemented"));
    return UGERR;
}

int KERNEL_MAXPROC(const char* cmd, int argc, const char** argv, SYSINFO_RESULT* result)
{
    SET_MSG_RESULT(result, xstrdup("not implemented"));
    return UGERR;
}

