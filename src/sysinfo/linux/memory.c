#include "sysinfo/sysinfo.h"
#include <sys/sysinfo.h>
#include "utils/string.h"



int VM_MEMORY_TOTAL(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    struct sysinfo  info;

    if (0 != sysinfo(&info)) {
        SET_MSG_RESULT(result,xstrdup("call sysinfo failed"));
        return UGERR;
    }

    SET_UI64_RESULT(result, bytesConvert((uint64_t)info.totalram * info.mem_unit,getParam(argc,argv,0)));

    return UGOK;
}

int VM_MEMORY_FREE(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    struct sysinfo  info;

    if (0 != sysinfo(&info)) {
        SET_MSG_RESULT(result,xstrdup("call sysinfo failed"));
        return UGERR;
    }

    SET_UI64_RESULT(result, bytesConvert((uint64_t)info.freeram * info.mem_unit,getParam(argc,argv,0)));

    return UGOK;
}

int VM_MEMORY_BUFFERS(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    struct sysinfo  info;

    if (0 != sysinfo(&info)) {
        SET_MSG_RESULT(result,xstrdup("call sysinfo failed"));
        return UGERR;
    }

    SET_UI64_RESULT(result, bytesConvert((uint64_t)info.bufferram * info.mem_unit,getParam(argc,argv,0)));

    return UGOK;
}

int VM_MEMORY_CACHED(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    FILE*        f;
    char*        t, c[MAX_STRING_LEN];
    uint64_t    res = 0;

    if (NULL == (f = fopen("/proc/meminfo", "r"))) {
        SET_MSG_RESULT(result,xstrdup("fopen /proc/meminfo failed"));
        return UGERR;
    }

    while (NULL != fgets(c, sizeof(c), f)) {
        if (0 == strncmp(c, "Cached:", 7)) {
            t = strtok(c, " ");
            t = strtok(NULL, " ");
            sscanf(t, UG_FS_UI64, &res);
            t = strtok(NULL, " ");

            if (0 == strncasecmp(t, "kb",2)) {
                res <<= 10;
            } else if (0 == strncasecmp(t, "mb",2)) {
                res <<= 20;
            } else if (0 == strncasecmp(t, "gb",2)) {
                res <<= 30;
            } else if (0 == strncasecmp(t, "tb",2)) {
                res <<= 40;
            }

            break;
        }
    }
    FILECLOSE(f);

    SET_UI64_RESULT(result, bytesConvert(res,getParam(argc,argv,0)));

    return UGOK;
}

int VM_MEMORY_USED(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    struct sysinfo  info;

    if (0 != sysinfo(&info)) {
        SET_MSG_RESULT(result,xstrdup("call sysinfo failed"));
        return UGERR;
    }

    SET_UI64_RESULT(result, bytesConvert((uint64_t)(info.totalram - info.freeram) * info.mem_unit,getParam(argc,argv,0)));

    return UGOK;
}

int VM_MEMORY_PUSED(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    struct sysinfo  info;

    if (0 != sysinfo(&info) || 0 == info.totalram) {
        SET_MSG_RESULT(result,xstrdup("call sysinfo failed"));
        return UGERR;
    }

    SET_DBL_RESULT(result, (info.totalram - info.freeram) / (double)info.totalram * 100);

    return UGOK;
}

int VM_MEMORY_AVAILABLE(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    struct sysinfo  info;
    SYSINFO_RESULT  result_tmp;

    if (0 != sysinfo(&info)) {
        SET_MSG_RESULT(result,xstrdup("call sysinfo failed"));
        return UGERR;
    }

    initResult(&result_tmp);

    if (UGOK != VM_MEMORY_CACHED(cmd,argc,argv,&result_tmp))
        return UGERR;

    SET_UI64_RESULT(result, bytesConvert((uint64_t)(info.freeram + info.bufferram) * info.mem_unit + result_tmp.ui64,getParam(argc,argv,0)));

    freeResult(&result_tmp);

    return UGOK;
}

int VM_MEMORY_PAVAILABLE(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    struct sysinfo  info;
    SYSINFO_RESULT  result_tmp;
    uint64_t    available, total;

    if (0 != sysinfo(&info)) {
        SET_MSG_RESULT(result,xstrdup("call sysinfo failed"));
        return UGERR;
    }

    initResult(&result_tmp);

    if (UGOK != VM_MEMORY_CACHED(cmd,argc,argv,&result_tmp)) {
        SET_MSG_RESULT(result,xstrdup("get cached memory failed"));
        return UGERR;
    }

    available = (uint64_t)(info.freeram + info.bufferram) * info.mem_unit + result_tmp.ui64;
    total = (uint64_t)info.totalram * info.mem_unit;

    if (0 == total)
        return UGERR;

    SET_DBL_RESULT(result, available / (double)total * 100);

    freeResult(&result_tmp);

    return UGOK;
}

int VM_MEMORY_SHARED(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
#ifdef KERNEL_2_4
    struct sysinfo  info;

    if (0 != sysinfo(&info)) {
        SET_MSG_RESULT(result, xstrdup("call sysinfo failed"));
        return UGERR;
    }

    SET_UI64_RESULT(result, bytesConvert((uint64_t)info.sharedram * info.mem_unit,getParam(argc,argv,0)));

    return UGOK;
#else
    SET_UI64_RESULT(result, (uint64_t)0);
    return UGOK;
#endif
}

