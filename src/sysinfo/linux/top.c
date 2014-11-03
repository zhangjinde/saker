#include "sysinfo/top.h"
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <dirent.h>
#include "utils/string.h"

#ifdef HAVE_ASM_PARAM_H
#include <asm/param.h>
#endif
#include <sys/param.h>    /* for HZ */
#ifndef HZ
#define HZ sysconf(_SC_CLK_TCK)
#endif
#define bytetok(x)  (((x) + 512) >> 10)

static inline char *skip_ws(const char *p) {
    while ((*p == ' ')) p++;
    return (char *)p;
}

static inline char *skip_token(const char *p) {
    while ((*p) == ' ') p++;
    while (*p && !(*p==' ')) p++;
    return (char *)p;
}

static void xfrm_cmdline(char *p, int len) {
    while (--len > 0) {
        if (*p == '\0') {
            *p = ' ';
        }
        p++;
    }
}

static uint64_t previous_total = 0;
#define TMPL_SHORTPROC "%*s %llu %llu %llu %llu"
#define TMPL_LONGPROC "%*s %llu %llu %llu %llu %llu %llu %llu %llu"

/* Determine if this kernel gives us "extended" statistics information in
 * /proc/stat.
 * Kernels around 2.5 and earlier only reported user, system, nice, and
 * idle values in proc stat.
 * Kernels around 2.6 and greater report these PLUS iowait, irq, softirq,
 * and steal */
static int determineLongStat(char *buf) {
    unsigned long long iowait = 0;
    /* scanf will either return -1 or 1 because there is only 1 assignment */
    if (sscanf(buf, "%*s %*d %*d %*d %*d %llu", &iowait) > 0) {
        return 1;
    }
    return 0;
}


static unsigned long long calc_cpu_total(void) {
    unsigned long long total = 0;
    unsigned long long t = 0;
    int rc;
    int ps;
    char line[MAX_STRING_LEN] = { 0 };
    unsigned long long cpu = 0;
    unsigned long long niceval = 0;
    unsigned long long systemval = 0;
    unsigned long long idle = 0;
    unsigned long long iowait = 0;
    unsigned long long irq = 0;
    unsigned long long softirq = 0;
    unsigned long long steal = 0;
    const char *mytemplate =NULL;

    ps = open("/proc/stat", O_RDONLY);
    rc = read(ps, line, sizeof(line));
    close(ps);
    if (rc < 0) {
        return 0;
    }
    mytemplate = determineLongStat(line) ? TMPL_LONGPROC : TMPL_SHORTPROC ;

    sscanf(line, mytemplate, &cpu, &niceval, &systemval, &idle, &iowait, &irq,
           &softirq, &steal);
    total = cpu + niceval + systemval + idle + iowait + irq + softirq + steal;

    t = total - previous_total;
    previous_total = total;

    return t;
}


static void update_procname(struct ProcessInfo *proc, char *cmd) {
    if (proc->name == NULL) {
        proc->name = xstrdup(cmd);
    } else if (strcmp(proc->name, cmd) != 0) {
        zfree(proc->name);
        proc->name = xstrdup(cmd);
    }
}

static void calcProcesssPCPU(struct ProcessInfo *proc, uint64_t elapsed) {
    if ((proc->pcpu = (proc->user_time + proc->kernel_time -  proc->previous_user_time - proc->previous_kernel_time ) / (double)elapsed) < 0.0001) {
        proc->pcpu = 0;
    }
    proc->pcpu = proc->pcpu * 100.0 * sysconf(_SC_NPROCESSORS_ONLN);
    proc->previous_user_time = proc->user_time;
    proc->previous_kernel_time = proc->kernel_time;
}

static void calcProcesssPMEM(struct ProcessInfo *proc) {
    static uint64_t totalram  = 0;
    if (totalram == 0) {
        struct sysinfo  info;
        if (0 != sysinfo(&info) || 0 == info.totalram) return ;
        totalram = info.totalram;
    }
    proc->pmem = proc->rss*1024 / (double)totalram * 100;
}

int updateProcess(struct ProcessInfo *proc) {
    int  ret = UGERR;
    char buffer[4096], *p, *q;
    int fd, len;
    int fullcmd;
    uint64_t cputime ;
    /* use getpagesize()  ? */
    uint64_t page = sysconf(_SC_PAGESIZE);
    pid_t  pid = proc->pid;

    struct stat process_stat;
    proc->state = 0;

    /* full cmd handling */
    sprintf(buffer, "/proc/%d/cmdline", pid);
    if ((fd = open(buffer, O_RDONLY)) != -1) {
        /* read command line data */
        /* (theres no sense in reading more than we can fit) */
        if ((len = read(fd, buffer, 255)) > 1) {
            buffer[len] = '\0';
            if (proc->fullname && (strcmp(proc->fullname, buffer)!=0)) {
                zfree(proc->fullname);
                proc->fullname = xstrdup(buffer);
            }
            xfrm_cmdline(buffer, len);
            update_procname(proc, buffer);
        } else {
            fullcmd = 0;
        }
        close(fd);
    }

    /* grab the shared memory size */
    sprintf(buffer, "/proc/%d/statm", pid);
    fd = open(buffer, O_RDONLY);
    len = read(fd, buffer, sizeof(buffer)-1);
    close(fd);
    buffer[len] = '\0';
    p = buffer;
    p = skip_token(p); /* skip size */
    p = skip_token(p); /* skip resident */
    proc->shared = (page*(strtoul(p, &p, 10))) >> 10;

    /* grab the proc stat info in one go */
    sprintf(buffer, "/proc/%d/stat", pid);

    fd = open(buffer, O_RDONLY);
    if (fd < 0) return UGERR;
    len = read(fd, buffer, sizeof(buffer)-1);
    if (fstat(fd, &process_stat) != 0) {
        return UGERR;
    }
    proc->uid = process_stat.st_uid;
    close(fd);
    buffer[len] = '\0';

    /* skip pid and locate command, which is in parentheses */
    if ((p = strchr(buffer, '(')) == NULL) {
        return UGERR;
    }
    if ((q = strrchr(++p, ')')) == NULL) {
        return UGERR;
    }

    /* set the procname */
    *q = '\0';

    update_procname(proc, p);
    /* scan the rest of the line */
    p = q+1;
    p = skip_ws(p);
    switch (*p++) { /* state */
    case 'R':
        proc->state = 1;
        break;
    case 'S':
        proc->state = 2;
        break;
    case 'D':
        proc->state = 3;
        break;
    case 'Z':
        proc->state = 4;
        break;
    case 'T':
        proc->state = 5;
        break;
    case 'W':
        proc->state = 6;
        break;
    case '\0':
        return UGERR;
    }

    p = skip_token(p);  /* skip ppid */
    p = skip_token(p);  /* skip pgrp */
    p = skip_token(p);  /* skip session */
    p = skip_token(p);  /* skip tty */
    p = skip_token(p);  /* skip tty pgrp */
    p = skip_token(p);  /* skip flags */
    p = skip_token(p);  /* skip min flt */
    p = skip_token(p);  /* skip cmin flt */
    p = skip_token(p);  /* skip maj flt */
    p = skip_token(p);  /* skip cmaj flt */


    proc->user_time = strtoul(p, &p, 10); /* utime */
    proc->kernel_time = strtoul(p, &p, 10); /* stime */

    p = skip_token(p);  /* skip cutime */
    p = skip_token(p);  /* skip cstime */
    /* jiffies -> seconds = 1 / HZ
     * HZ is defined in "asm/param.h"  and it is usually 1/100s but on
     * alpha system it is 1/1024s
     */
    cputime     = ((float)(proc->time) * 10.0) / HZ;

    proc->pri = strtol(p, &p, 10);  /* priority */
    proc->nice = strtol(p, &p, 10);   /* nice */
    proc->threads = strtol(p, &p, 10);  /* threads */

    p = skip_token(p);  /* skip it_real_val */
    proc->start_time = strtoul(p, &p, 10);  /* start_time */
    // get_starttime() + (time_t)(stat_item_starttime / HZ);
    proc->vsize = bytetok(strtoul(p, &p, 10)); /* vsize */
    proc->rss = strtoul(p, &p, 10)*page>>10;  /* rss */

    ret = UGOK;
    return ret;
}


int topUpdate() {
    DIR *dir;
    pid_t pid;
    struct dirent *entry;
    uint64_t  elapsed = calc_cpu_total();

    if (!(dir = opendir("/proc"))) {
        return 1;
    }
    ++g_time;


    /* Get list of processes from /proc directory */
    while ((entry = readdir(dir))) {
        if (!entry) {
            closedir(dir);
            return UGERR;
        }

        if (sscanf(entry->d_name, "%d", &pid) > 0) {
            struct ProcessInfo *p;

            p = findProcess(pid);
            if (!p) {
                p = newProcess(pid);
            }
            /* Mark process as up-to-date. */
            p->time_stamp = g_time;

            updateProcess(p);
            /* Calc process cpu usage */
            calcProcesssPCPU(p, elapsed);

            calcProcesssPMEM(p);
        }
    }
    closedir(dir);
    processCleanup();
    return UGOK;
}




