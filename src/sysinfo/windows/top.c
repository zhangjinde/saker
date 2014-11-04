#include "sysinfo/top.h"

#include "utils/process.h"



int   updateProcess(struct ProcessInfo *proc) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, proc->pid);

    return 0;
}


int   topUpdate(void) {
    struct ProcessInfo *p = NULL;
    pid_t pid;
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;
    HANDLE hProcess;
    char procname[1024]= {0};
    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) ) {
        return UGERR;
    }

    // Calculate how many process identifiers were returned.
    cProcesses = cbNeeded / sizeof(DWORD);
    g_time++;
    for ( i = 0; i < cProcesses; i++ ) {
        hProcess = OpenProcess(  PROCESS_QUERY_INFORMATION |
                                 PROCESS_VM_READ,
                                 FALSE, aProcesses[i] );
        if (NULL == hProcess)
            continue;
        if (get_procname(hProcess,procname, 1024) != UGOK) {
            hProcess = NULL;
            CloseHandle(hProcess);
            continue;
        }
        pid = aProcesses[i];
        p = findProcess(pid);
        if (!p) {
            p = newProcess(pid);
        }
        /* Mark process as up-to-date. */
        p->time_stamp = g_time;

        updateProcess(p);

        /* Calc process cpu usage */
        // calcProcesssPCPU(p, elapsed);

        // calcProcesssPMEM(p);

        CloseHandle(hProcess);
        hProcess = NULL;

    }

    CloseHandle(hProcess);

    return UGOK;
}


