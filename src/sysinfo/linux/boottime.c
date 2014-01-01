#include "sysinfo/sysinfo.h"
#include "utils/string.h"

int SYSTEM_BOOTTIME(const char* cmd, int argc, const char** argv, SYSINFO_RESULT* result)
{
    FILE*        f;
    char        buf[MAX_STRING_LEN];
    int     ret = UGERR;
    unsigned long   value;

    if (NULL == (f = fopen("/proc/stat", "r"))) {
        SET_MSG_RESULT(result,xstrdup("fopen /proc/stat failed"));
        return ret;
    }

    /* find boot time entry "btime [boot time]" */
    while (NULL != fgets(buf, MAX_STRING_LEN, f)) {
        if (1 == sscanf(buf, "btime %lu", &value)) {
            SET_UI64_RESULT(result, value);

            ret = UGOK;

            break;
        }
    }

    FILECLOSE(f);

    return ret;
}
