#ifndef _TOP__H_
#define _TOP__H_



#include "utils/udict.h"
#include "utils/ulist.h"
#include "common/common.h"

#ifdef _WIN32
#define uid_t int
#endif

extern uint32_t g_time;

struct ProcessInfo {
    pid_t pid;
    char* name;
    char* fullname;
    uid_t uid;
    float amount;
    // User and kernel times are in hundredths of seconds
    unsigned long user_time;
    unsigned long total;
    unsigned long kernel_time;
    unsigned long previous_user_time;
    unsigned long previous_kernel_time;
    unsigned long previous_cputime ;
    unsigned long total_cpu_time;
    unsigned long long vsize;  /* in k */
    unsigned long long rss;  /* in k */
    unsigned long long shared; /* in k */
    
    double pcpu;
    double pmem;
  #ifdef IOSTATS
    unsigned long long read_bytes;
    unsigned long long previous_read_bytes;
    unsigned long long write_bytes;
    unsigned long long previous_write_bytes;
    float io_perc;
  #endif
    uint32_t time_stamp;
    unsigned int counted;
    unsigned int changed;
    
    int pri, nice, threads;
    int state; /* process state  sleeping running stopped or zombie */
    unsigned long time;
    unsigned long start_time;
};


/* just for platform */

struct ProcessInfo* newProcess(pid_t pid);

struct ProcessInfo* findProcess(pid_t pid);

void  deleteProcess(pid_t pid);

void  processCleanup(void);

int   topIsRuning(void);

/* end for platform */

void  freeProcess(struct ProcessInfo* p);

int   updateProcess(struct ProcessInfo* proc);

struct ProcessInfo* getProcessInfoByID(pid_t pid);

int   topUpdate(void);

int   topIsRuning(void);

dict* createTop();

void  destroyTop(dict* p);


#endif
