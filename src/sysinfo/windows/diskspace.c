#include "sysinfo/sysinfo.h"
#include <assert.h>



int VFS_FS_SIZE(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    char*        mode = NULL;
    ULARGE_INTEGER  freeBytes, totalBytes;
    mode = strrchr(cmd,'.')+1;

    if (0 == GetDiskFreeSpaceEx(getParam(argc,argv,0), &freeBytes, &totalBytes, NULL)) {
        return UGERR;
    }

    if ('\0' == *mode || 0 == strcmp(mode, "total"))    /* default parameter */
        SET_UI64_RESULT(result, bytesConvert(totalBytes.QuadPart,getParam(argc,argv,1)));
    else if (0 == strcmp(mode, "free"))
        SET_UI64_RESULT(result, bytesConvert(freeBytes.QuadPart,getParam(argc,argv,1)));
    else if (0 == strcmp(mode, "used"))
        SET_UI64_RESULT(result, bytesConvert(totalBytes.QuadPart - freeBytes.QuadPart,getParam(argc,argv,1)));
    else if (0 == strcmp(mode, "pfree"))
        SET_DBL_RESULT(result, (double)(__int64)freeBytes.QuadPart * 100. / (double)(__int64)totalBytes.QuadPart);
    else if (0 == strcmp(mode, "pused"))
        SET_DBL_RESULT(result, (double)((__int64)totalBytes.QuadPart - (__int64)freeBytes.QuadPart) * 100. /
                       (double)(__int64)totalBytes.QuadPart);
    else
        return UGERR;

    return UGOK;
}

