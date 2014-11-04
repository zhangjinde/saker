#include "sysinfo/sysinfo.h"
#include "utils/string.h"


int VM_MEMORY_TOTAL(const char *cmd, int argc,const char **argv,SYSINFO_RESULT *result) {
    MEMORYSTATUSEX      ms_ex;
    ms_ex.dwLength = sizeof (ms_ex);
    GlobalMemoryStatusEx(&ms_ex);
    SET_UI64_RESULT(result,  bytesConvert(ms_ex.ullTotalPhys,getParam(argc,argv,0)));
    return UGOK;
}

int VM_MEMORY_FREE(const char *cmd, int argc,const char **argv,SYSINFO_RESULT *result) {
    MEMORYSTATUSEX      ms_ex;
    ms_ex.dwLength = sizeof (ms_ex);
    GlobalMemoryStatusEx(&ms_ex);
    SET_UI64_RESULT(result, bytesConvert(ms_ex.ullAvailPhys,getParam(argc,argv,0)));

    return UGOK;
}

int VM_MEMORY_BUFFERS(const char *cmd, int argc,const char **argv,SYSINFO_RESULT *result) {
    SET_MSG_RESULT(result,xstrdup("not implemented"));
    return UGERR;
}

int VM_MEMORY_CACHED(const char *cmd, int argc,const char **argv,SYSINFO_RESULT *result) {

    PERFORMANCE_INFORMATION pfi;
    GetPerformanceInfo(&pfi, sizeof(PERFORMANCE_INFORMATION));

    SET_UI64_RESULT(result, bytesConvert(pfi.SystemCache * pfi.PageSize,getParam(argc,argv,0)));

    return UGOK;
}

int VM_MEMORY_USED(const char *cmd, int argc,const char **argv,SYSINFO_RESULT *result) {
    MEMORYSTATUSEX      ms_ex;
    ms_ex.dwLength = sizeof (ms_ex);
    GlobalMemoryStatusEx(&ms_ex);
    SET_UI64_RESULT(result, bytesConvert(ms_ex.ullTotalPhys - ms_ex.ullAvailPhys,getParam(argc,argv,0)));
    return UGOK;
}

int VM_MEMORY_PUSED(const char *cmd, int argc,const char **argv,SYSINFO_RESULT *result) {

    MEMORYSTATUSEX      ms_ex;
    ms_ex.dwLength = sizeof (ms_ex);
    GlobalMemoryStatusEx(&ms_ex);
    SET_DBL_RESULT(result,(ms_ex.ullTotalPhys - ms_ex.ullAvailPhys) / (double)ms_ex.ullTotalPhys * 100);

    return UGOK;
}

int VM_MEMORY_AVAILABLE(const char *cmd, int argc,const char **argv,SYSINFO_RESULT *result) {
    MEMORYSTATUSEX      ms_ex;
    ms_ex.dwLength = sizeof (ms_ex);
    GlobalMemoryStatusEx(&ms_ex);
    SET_UI64_RESULT(result, bytesConvert(ms_ex.ullAvailPhys,getParam(argc,argv,0)));

    return UGOK;
}

int VM_MEMORY_PAVAILABLE(const char *cmd, int argc,const char **argv,SYSINFO_RESULT *result) {
    MEMORYSTATUSEX      ms_ex;
    ms_ex.dwLength = sizeof (ms_ex);
    GlobalMemoryStatusEx(&ms_ex);
    SET_DBL_RESULT(result,  ms_ex.ullAvailPhys / (double)ms_ex.ullTotalPhys * 100);
    return UGOK;
}

int VM_MEMORY_SHARED(const char *cmd, int argc,const char **argv,SYSINFO_RESULT *result) {
    SET_MSG_RESULT(result,xstrdup("not implemented"));
    return UGERR;
}
