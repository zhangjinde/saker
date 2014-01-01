

#include "sysinfo/sysinfo.h"
#include "sysinfo/top.h"
#include "utils/error.h"
#include "utils/string.h"
#include "utils/process.h"
#include "utils/sds.h"


static HANDLE openProcessByPID(pid_t pid)
{
    return OpenProcess(PROCESS_QUERY_INFORMATION |
                       PROCESS_VM_READ, FALSE, pid);
}

#if 0
static HANDLE openProcessByName(const char* name, pid_t* pid)
{
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;
    HANDLE hProcess;
    char procname[1024]= {0};
    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
        return NULL;

    // Calculate how many process identifiers were returned.
    cProcesses = cbNeeded / sizeof(DWORD);

    for ( i = 0; i < cProcesses; i++ ) {
        hProcess = OpenProcess(  PROCESS_QUERY_INFORMATION |
                                 PROCESS_VM_READ,
                                 FALSE, aProcesses[i] );
        if (NULL == hProcess)
            continue;
        if(get_proc_name(hProcess,procname,1024) != UGOK) {
            hProcess = NULL;
            CloseHandle(hProcess);
            continue;
        }
        if(strcmp(procname,name) == 0) {
            if(pid) *pid = aProcesses[i];
            break;
        }
        CloseHandle(hProcess);
        hProcess = NULL;

    }
    return hProcess;

}
#endif

int PROC_PID(const char* cmd,int argc, const char** argv, SYSINFO_RESULT* result)
{
    const char* name= getParam(argc, argv, 0);
    sds pidstr = sdsnew("[");
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;
    HANDLE hProcess;
    char procname[1024]= {0};
    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) ) {
        SET_MSG_RESULT(result, "EnumProcesses failed");
        return UGERR;
    }

    // Calculate how many process identifiers were returned.
    cProcesses = cbNeeded / sizeof(DWORD);

    for ( i = 0; i < cProcesses; i++ ) {
        hProcess = OpenProcess(  PROCESS_QUERY_INFORMATION |
                                 PROCESS_VM_READ,
                                 FALSE, aProcesses[i] );
        if (NULL == hProcess)
            continue;
        if(get_procname(hProcess,procname, 1024) != UGOK) {
            hProcess = NULL;
            CloseHandle(hProcess);
            continue;
        }
        if(strcmp(procname,name) == 0) {
            pidstr = sdscatprintf(pidstr, "%d", aProcesses[i]);
            pidstr = sdscat(pidstr, ",");
        }
        CloseHandle(hProcess);
        hProcess = NULL;

    }

    CloseHandle(hProcess);
    pidstr[sdslen(pidstr)-1] = ']';

    if (sdslen(pidstr) == 1) {
        SET_MSG_RESULT(result, xstrdup("can not find the procname"));
        sdsfree(pidstr);
        return UGERR;
    }

    SET_STR_RESULT(result, xstrdup(pidstr));
    sdsfree(pidstr);

    return UGOK;
}

int PROC_MEMORY_USED(const char* cmd, int argc, const char** argv, SYSINFO_RESULT* result)
{
    PROCESS_MEMORY_COUNTERS pmc;
    HANDLE hProcess ;
    int    ret = UGERR;

    if(xstrisdigit(getParam(argc,argv,0))==UGOK) {
        hProcess = openProcessByPID(atoi(getParam(argc,argv,0)));
    } else {
        SET_MSG_RESULT(result, "the param must pid");
        return UGERR;
    }
    /*
       else {
             hProcess = openProcessByName(getParam(argc,argv,0),NULL);
        }
        */

    if(hProcess == NULL ) {
        return UGERR;
    }
    if(GetProcessMemoryInfo(hProcess,&pmc,sizeof(pmc))) {
        SET_UI64_RESULT(result,bytesConvert(pmc.WorkingSetSize,getParam(argc,argv,1)));
        ret = UGOK;
    }
    CloseHandle(hProcess);

    return ret;
}

int PROC_MEMORY_PUSED(const char* cmd,int argc,const char** argv,SYSINFO_RESULT* result)
{
    //CreateToolhelp32Snapshot 根据 进程名
    PROCESS_MEMORY_COUNTERS pmc;
    MEMORYSTATUSEX      ms_ex;
    HANDLE hProcess ;
    int    ret = UGERR;
    if (xstrisdigit(getParam(argc,argv,0)) == UGOK) {
        hProcess = openProcessByPID(atoi(getParam(argc,argv,0)));
    } else {
        SET_MSG_RESULT(result, "the param must pid");
        return UGERR;
    }

    if (hProcess == NULL ) {
        return UGERR;
    }
    if (GetProcessMemoryInfo(hProcess,&pmc,sizeof(pmc))) {
        GlobalMemoryStatusEx(&ms_ex);
        SET_DBL_RESULT(result,pmc.WorkingSetSize/(double)ms_ex.ullTotalPhys*100);
        ret =  UGOK;
    }
    CloseHandle(hProcess);
    return ret;
}

int PROC_CPU_LOAD(const char* cmd,int argc,const char** argv,SYSINFO_RESULT* result)
{
    SET_MSG_RESULT(result, xstrdup("not implemented"));
    return UGERR;
}

int PROC_STATINFO(const char* cmd,int argc,const char** argv,SYSINFO_RESULT* result)
{
    int  ret = UGERR;
    pid_t  pid = 0;
    const char* pidstr = NULL;
    char* rst = NULL;
    struct ProcessInfo* proc  = NULL;
    if(NULL == (pidstr=getParam(argc,argv,0))) {
        SET_MSG_RESULT(result, xstrdup("called must have param"));
        return ret;
    }
    pid = atoi(pidstr);
   
    if (topIsRuning()) {
        proc = getProcessInfoByID(pid);
        if (!proc) {
            SET_MSG_RESULT(result, xstrprintf("cannot found the process %d", pid));
            return ret;
        }
    } else {
        proc = zmalloc(sizeof(struct ProcessInfo));
        memset(proc, 0, sizeof(struct ProcessInfo));
        proc->pid = pid;
        if (updateProcess(proc) == UGERR) {
            SET_MSG_RESULT(result, xstrprintf("cannot found the process or read stat failed %d ", pid));
            return ret;
        }
    }

    /* convert to json */
    rst = xstrprintf("{\"PID\":%d,\"UID\":%d,\"Name\":\"%s\",\"Fullname\":\"%s\",\"Threads\":%d,\"VIRT\":%lu,\"RES\":%lu,\"SHR:\":%lu,\"State\":%d,\"PCPU\":%f,\"PMEM\":%f}",
        proc->pid,
        proc->uid,
        proc->name,
        proc->fullname,
        proc->threads,
        proc->vsize,
        proc->rss,
        proc->shared,
        proc->state,
        proc->pcpu,
        proc->pmem);
    SET_STR_RESULT(result, rst);
    ret = UGOK;

    if (!topIsRuning()) {
        freeProcess(proc);
    }
    return ret;
    return UGERR;
}

