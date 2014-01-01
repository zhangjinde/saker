#include "sysinfo/sysinfo.h"


static int  read_uint64_from_procfs(const char* path, uint64_t* value)
{
    int ret = UGERR;
    char    line[MAX_STRING_LEN];
    FILE*    f;

    if (NULL != (f = fopen(path, "r"))) {
        if (NULL != fgets(line, sizeof(line), f)) {
            if (1 == sscanf(line, UG_FS_UI64 "\n", value))
                ret = UGOK;
        }
        fclose(f);
    }

    return ret;
}

int KERNEL_MAXFILES(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    int     ret = UGERR;
    uint64_t    value;

    if (UGOK == read_uint64_from_procfs("/proc/sys/fs/file-max", &value)) {
        SET_UI64_RESULT(result, value);
        ret = UGOK;
    }

    return ret;
}

int KERNEL_MAXPROC(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    int     ret = UGERR;
    uint64_t    value;

    if (UGOK == read_uint64_from_procfs("/proc/sys/kernel/pid_max", &value)) {
        SET_UI64_RESULT(result, value);
        ret = UGOK;
    }

    return ret;
}

