#include "sysinfo/sysinfo.h"
#include <stdlib.h>
#include "utils/string.h"


#define CPU_AVG1        0
#define CPU_AVG5        1
#define CPU_AVG15       2
#define CPU_AVG_COUNT       3


/**
* /brief linux cpu's num can config
*        online  --
*        max    --
*       config dir : /sys/devices/system/cpu/
*/

int SYSTEM_CPU_NUM_ONLINE(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    long    ncpu;
    if (-1 == (ncpu = sysconf(_SC_NPROCESSORS_ONLN))) {
        SET_MSG_RESULT(result,xstrdup("call sysconf failed"));
        return UGERR;
    }

    SET_UI64_RESULT(result, ncpu);

    return UGOK;
}


int SYSTEM_CPU_NUM_MAX(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    long    ncpu;
    if (-1 == (ncpu = sysconf(_SC_NPROCESSORS_CONF))) {
        SET_MSG_RESULT(result,xstrdup("call sysconf failed"));
        return UGERR;
    }

    SET_UI64_RESULT(result, ncpu);

    return UGOK;
}


int SYSTEM_CPU_LOAD(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    int mode, per_cpu = 0, cpu_num;
    double  load[CPU_AVG_COUNT], value;

    const char* avgparam = getParam(argc,argv,0);
    if (avgparam==NULL || strcmp("avg1",avgparam)==0)  {
        mode = CPU_AVG1;
    } else if (strcmp("avg5",avgparam)==0) {
        mode = CPU_AVG5;
    } else if (strcmp("avg15",avgparam) == 0) {
        mode = CPU_AVG15;
    } else {
        SET_MSG_RESULT(result, xstrdup("wrong param"));
        return UGERR;
    }

    if (mode >= getloadavg(load, 3)) {
        SET_MSG_RESULT(result,xstrdup("call getloadavg failed"));
        return UGERR;
    }

    value = load[mode];

    if (1 == per_cpu) {
        /*    just count all cpu's loadavg    */
        if (0 >= (cpu_num = sysconf(_SC_NPROCESSORS_ONLN))) {
            SET_MSG_RESULT(result,xstrdup("call sysconf failed"));
            return UGERR;
        }
        value /= cpu_num;
    }

    SET_DBL_RESULT(result, value);

    return UGOK;

}


int SYSTEM_CPU_UTIL(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{

    SET_MSG_RESULT(result,xstrdup("not implemented"));
    return UGERR;
}

