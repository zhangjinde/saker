#include "sysinfo/sysinfo.h"

#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>

#include "sysinfo/top.h"
#include "utils/string.h"
#include "utils/sds.h"
#include "common/types.h"

#ifdef HAVE_ASM_PARAM_H
#include <asm/param.h>
#endif

#ifndef HZ
#define HZ sysconf(_SC_CLK_TCK)
#endif


#define bytetok(x)  (((x) + 512) >> 10)

/**
 * Get system start time
 * @return seconds since unix epoch
 */
static time_t get_starttime() {
    char   buf[1024];
    double up = 0;
    int fd,bytes;
    if ((fd = open("/proc/uptime", O_RDONLY)) < 0) return 0;
    if ((bytes = (int)read(fd, buf, sizeof(buf)-1)) < 0) {
        close(fd);
        return 0;
    }
    buf[bytes]='\0';
    close(fd);
    if (sscanf(buf, "%lf", &up) != 1) {
        return 0;
    }
    return time(NULL) - (time_t)up;
}

int proc_owner(int pid) {
    struct stat sb;
    char buffer[32];
    sprintf(buffer, "%d", pid);

    if (stat(buffer, &sb) < 0)
        return -1;
    return (int)sb.st_uid;
}

static FILE *open_proc_file(const char *filename) {
    struct stat s;

    if (0 != stat(filename, &s))
        return NULL;

    return fopen(filename, "r");
}

static int  get_cmdline(FILE *f_cmd, char **line, size_t *line_offset) {
    size_t  line_alloc = MAX_STRING_LEN, n;

    rewind(f_cmd);

    *line = (char *)zmalloc(line_alloc + 2);
    *line_offset = 0;

    while (0 != (n = fread(*line + *line_offset, 1, line_alloc - *line_offset, f_cmd))) {
        *line_offset += n;

        if (0 != feof(f_cmd))
            break;

        line_alloc *= 2;
        *line = zrealloc(*line, line_alloc + 2);
    }

    if (0 == ferror(f_cmd)) {
        if (0 == *line_offset || '\0' != (*line)[*line_offset - 1])
            (*line)[(*line_offset)++] = '\0';
        if (1 == *line_offset || '\0' != (*line)[*line_offset - 2])
            (*line)[(*line_offset)++] = '\0';

        return UGOK;
    }

    zfree(*line);
    *line = NULL;

    return UGERR;
}

static int  cmp_status(FILE *f_stat, const char *procname) {
    char    tmp[MAX_STRING_LEN];

    rewind(f_stat);

    while (NULL != fgets(tmp, sizeof(tmp), f_stat)) {
        if (0 != strncmp(tmp, "Name:\t", 6))
            continue;

        xstrrtrim(tmp + 6, "\n");
        if (0 == strcmp(tmp + 6, procname))
            return UGOK;
        break;
    }

    return UGERR;
}

static int  check_procname(FILE *f_cmd, FILE *f_stat, const char *procname) {
    char    *tmp = NULL, *p;
    size_t  l;
    int ret = UGOK;

    if ('\0' == *procname)
        return UGOK;

    /* process name in /proc/[pid]/status contains limited number of characters */
    if (UGOK == cmp_status(f_stat, procname))
        return UGOK;

    if (UGOK == get_cmdline(f_cmd, &tmp, &l)) {
        if (NULL == (p = strrchr(tmp, '/')))
            p = tmp;
        else
            p++;

        if (0 == strcmp(p, procname))
            goto clean;
    }

    ret = UGERR;
clean:
    if(NULL!=tmp)
        zfree(tmp);

    return ret;
}

/*
static int  check_user(FILE *f_stat, struct passwd *usrinfo)
{
    char    tmp[MAX_STRING_LEN], *p, *p1;
    uid_t   uid;

    if (NULL == usrinfo)
        return UGOK;

    rewind(f_stat);

    while (NULL != fgets(tmp, sizeof(tmp), f_stat))
    {
        if (0 != strncmp(tmp, "Uid:\t", 5))
            continue;

        p = tmp + 5;

        if (NULL != (p1 = strchr(p, '\t')))
            *p1 = '\0';

        uid = (uid_t)atoi(p);

        if (usrinfo->pw_uid == uid)
            return UGOK;
        break;
    }

    return UGERR;
}
*/

static int  check_procstate(FILE *f_stat, int proc_stat) {
    char    tmp[MAX_STRING_LEN], *p;

    if (0 == proc_stat)
        return UGOK;

    rewind(f_stat);

    while (NULL != fgets(tmp, sizeof(tmp), f_stat)) {
        if (0 != strncmp(tmp, "State:\t", 7))
            continue;

        p = tmp + 7;

        switch (proc_stat) {
        case 1:
            return ('R' == *p) ? UGOK : UGERR;
        case 2:
            return ('S' == *p) ? UGOK : UGERR;
        case 3:
            return ('Z' == *p) ? UGOK : UGERR;
        default:
            return UGERR;
        }
    }

    return UGERR;
}

/*
 * @param argv[0]  =  processname
 */
int PROC_PID(const char *cmd,int argc,const char **argv,SYSINFO_RESULT *result) {
    int    ret = UGERR;
    const char *procname = getParam(argc,argv,0);

    // ! may be not enough !
    sds pidstr = sdsnew("[");

    char buff[MAX_STRING_LEN]= {0};
    int    flag = 0;
    FILE *f_cmd = NULL;
    FILE *f_stat = NULL;
    DIR  *dir;
    struct dirent   *entries;
    if( procname == NULL) {
        char *str =xstrdup("invaild param");
        SET_MSG_RESULT(result, str);
        return ret;
    }

    if( strcmp(procname,"all") == 0) {
        flag = 1;
    }

    if (NULL == (dir = opendir("/proc"))) {
        SET_MSG_RESULT(result, xstrdup("open /proc failed"));
        return ret;
    }
    while (NULL != (entries = readdir(dir))) {
        /* Self is a symbolic link. It leads to incorrect results for proc_cnt[zabbix_agentd]. */
        /* Better approach: check if /proc/x/ is symbolic link. */
        FILECLOSE(f_cmd);
        FILECLOSE(f_stat);
        if (UGERR == xstrisdigit(entries->d_name))
            continue;

        if (0 == strncmp(entries->d_name, "self", MAX_STRING_LEN))
            continue;

        snprintf(buff, sizeof(buff), "/proc/%s/cmdline", entries->d_name);

        if (NULL == (f_cmd = open_proc_file(buff)))
            continue;

        snprintf(buff, sizeof(buff), "/proc/%s/status", entries->d_name);

        if (NULL == (f_stat = open_proc_file(buff)))
            continue;

        if (flag == 0) {
            if (UGERR == check_procname(f_cmd, f_stat, procname))
                continue;
        }


        //if (UGERR == check_user(f_stat, usrinfo))
        //  continue;

        //if (UGERR == check_proccomm(f_cmd, proccomm))
        //  continue;
        pidstr = sdscat(pidstr, entries->d_name);
        pidstr = sdscat(pidstr, ",");
        rewind(f_stat);

    }
    FILECLOSE(f_cmd);
    FILECLOSE(f_stat);

    closedir(dir);

    pidstr[sdslen(pidstr)-1] = ']';
    if (sdslen(pidstr) == 1) {
        SET_MSG_RESULT(result, xstrdup("can not find the procname"));
        sdsfree(pidstr);
        return ret;
    }

    SET_STR_RESULT(result, xstrdup(pidstr));
    sdsfree(pidstr);
    return UGOK;
}
/*
 * @param cmd "system.proc.mem.rss"
 * @param argv[0] pid
 * @param argv[1] mb|kb|gb|nil
 */
int PROC_MEMORY_RSS(const char *cmd,int argc,const char **argv,SYSINFO_RESULT *result) {
    int    ret = UGERR;
    uint64_t memsize = 0;
    char   tmp[MAX_STRING_LEN]= {0};
    char   buff[MAX_STRING_LEN]= {0};
    FILE  *fp = NULL;
    const char *pidstr;
    char *p, *p1;
    int    pid = 0;
    if(NULL == (pidstr=getParam(argc,argv,0))) {
        SET_MSG_RESULT(result, xstrdup("called must have param"));
        return ret;
    }
    if(UGERR == xstrisdigit(pidstr)) {
        SET_MSG_RESULT(result, xstrdup("param must pid"));
        return ret;
    }

    snprintf(buff, sizeof(buff), "/proc/%s/status", pidstr);

    if (NULL == (fp = open_proc_file(buff))) {
        SET_MSG_RESULT(result, xstrprintf("cannot found the process %d", pid));
        return UGERR;
    }

    while (NULL != fgets(tmp, sizeof(tmp), fp)) {
        if (0 != strncmp(tmp, "VmRSS:\t", 7))
            continue;

        p = tmp + 8;

        if (NULL == (p1 = strrchr(p, ' ')))
            continue;

        *p1++ = '\0';

        UG_STR2UINT64(memsize, p);

        xstrrtrim(p1, "\n");

        if (0 == xstrcasecmp(p1, "kB"))
            memsize <<= 10;
        else if(0 == xstrcasecmp(p1, "mB"))
            memsize <<= 20;
        else if(0 == xstrcasecmp(p1, "GB"))
            memsize <<= 30;
        else if(0 == xstrcasecmp(p1, "TB"))
            memsize <<= 40;

        break;

    }
    FILECLOSE(fp);
    SET_UI64_RESULT(result,bytesConvert(memsize,getParam(argc,argv,1)));
    return UGOK;
}

/// argv[0] == pid argv[1] == mb|kb|gb|nil
int PROC_MEMORY_USED(const char *cmd,int argc,const char **argv,SYSINFO_RESULT *result) {
    int    ret = UGERR;
    uint64_t memsize = 0;
    char   tmp[MAX_STRING_LEN]= {0};
    char   buff[MAX_STRING_LEN]= {0};
    FILE  *fp = NULL;
    const char *pidstr;
    char *p, *p1;
    int    pid = 0;
    if(NULL == (pidstr=getParam(argc,argv,0))) {
        SET_MSG_RESULT(result, xstrdup("called must have param"));
        return ret;
    }
    if(UGERR == xstrisdigit(pidstr)) {
        SET_MSG_RESULT(result, xstrdup("param must pid"));
        return ret;
    }

    snprintf(buff, sizeof(buff), "/proc/%s/status", pidstr);

    if (NULL == (fp = open_proc_file(buff))) {
        SET_MSG_RESULT(result, xstrprintf("cannot found the process %d", pid));
        return UGERR;
    }

    while (NULL != fgets(tmp, sizeof(tmp), fp)) {
        if (0 != strncmp(tmp, "VmSize:\t", 8))
            continue;

        p = tmp + 8;

        if (NULL == (p1 = strrchr(p, ' ')))
            continue;

        *p1++ = '\0';

        UG_STR2UINT64(memsize, p);

        xstrrtrim(p1, "\n");

        if (0 == xstrcasecmp(p1, "kB"))
            memsize <<= 10;
        else if(0 == xstrcasecmp(p1, "mB"))
            memsize <<= 20;
        else if(0 == xstrcasecmp(p1, "GB"))
            memsize <<= 30;
        else if(0 == xstrcasecmp(p1, "TB"))
            memsize <<= 40;

        break;

    }
    FILECLOSE(fp);
    SET_UI64_RESULT(result,bytesConvert(memsize,getParam(argc,argv,1)));
    return UGOK;
}

/// argv[0] == pid
int PROC_MEMORY_PUSED(const char *cmd,int argc,const char **argv,SYSINFO_RESULT *result) {
    SYSINFO_RESULT tmpresult;
    const char *inparam = argv[0];
    struct sysinfo  info;

    if (0 != sysinfo(&info) || 0 == info.totalram)
        return UGERR;

    if ( UGERR == PROC_MEMORY_USED(NULL,1,&inparam,&tmpresult)) {
        return UGERR;
    }

    SET_DBL_RESULT(result,GET_UI64_RESULT(&tmpresult)/ (double)info.totalram * 100);

    return UGOK;
}


/// argv[0] == pid
int PROC_CPU_LOAD(const char *cmd,int argc,const char **argv,SYSINFO_RESULT *result) {
    int    ret = UGERR;
    /* todo here */
    return ret;
}

int PROC_STATINFO(const char *cmd,int argc,const char **argv,SYSINFO_RESULT *result) {
    int  ret = UGERR;
    pid_t  pid = 0;
    const char *pidstr;
    char *rst = NULL;
    struct ProcessInfo *proc  = NULL;
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
}
