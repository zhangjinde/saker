#include "process.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "common/common.h"
#include "utils/logger.h"
#include "common/defines.h"
#include "utils/error.h"
#include "utils/string.h"
#include "utils/file.h"
#include "utils/debug.h"

#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

static int lock_file(int fd) {
    struct flock fl;
    (void) memset(&fl, 0, sizeof(fl));
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;

    return(fcntl(fd, F_SETLK, &fl));
}

static int get_procname(pid_t pid, char *procname, int len) {
    char tmp[MAX_STRING_LEN] = {0};
    char buff[MAX_STRING_LEN] = {0};
    FILE *fp =  NULL;
    struct stat s;
    ugAssert(procname != NULL);
    *procname = 0;

    snprintf(buff, sizeof(buff), "/proc/%d/status", pid);

    if (0 != stat(buff, &s))
        return UGERR;

    if((fp = fopen(buff, "r")) == NULL) return UGERR;

    while (NULL != fgets(tmp, sizeof(tmp), fp)) {
        if (0 == strncmp(tmp, "State:\t", 7)) {
            char *proc_stat = tmp + 7;
            if (*proc_stat == 'Z') {
                *procname = 0;
                break;
            } else if (*procname != 0) {
                break;
            }
        } else if (0 == strncmp(tmp, "Name:\t", 6)) {
            xstrrtrim(tmp + 6, "\n");
            strncpy(procname, tmp+6, len);
        }
    }

    FILECLOSE(fp);

    /* TODO can match cmdline */
    if (strlen(procname) == 0) return UGERR;
    return UGOK;
}

void daemonize() {
    int i;
    pid_t pid;

    /*
     * Clear file creation mask
     */
    umask(0);

    /*
     * Become a session leader to lose our controlling terminal
     */
    if ((pid = fork ()) < 0) {
        LOG_ERROR("Cannot fork of a new process");
        _exit (1);

    } else if(pid != 0) {

        _exit(0);

    }

    setsid();

    if ((pid= fork ()) < 0) {
        LOG_ERROR("Cannot fork of a new process");
        exit (1);

    } else if(pid != 0) {

        _exit(0);

    }


    /*
     * Change current directory to the root so that other file systems
     * can be unmounted while we're running
     */
    /*
        if(chdir("/") < 0) {
            LOG_ERROR("Cannot chdir to '/' ");
            exit(1);
        }
    */
    for (i= 0; i < 3; i++) {
        if (close(i) == -1 || open("/dev/null", O_RDWR) != i) {
            LOG_ERROR("Cannot reopen standard file descriptor (%d) -- \n", i);
        }
    }
}

int pidfile_create( const char *pidfile,const char *appname,pid_t pid ) {
    int     fd;
    char    buf[MAX_STRING_LEN]= {0};

    fd = open(pidfile, O_RDWR|O_CREAT, LOCKMODE);
    if (fd < 0) {
        return UGERR;
    }
    if (lock_file(fd) < 0) {
        return UGERR;
    }

    ftruncate(fd, 0);
    sprintf(buf, "%d\n", pid != 0 ? pid : getpid());
    /* sprintf(buf+strlen(buf), "%s", appname);  */
    write(fd, buf, strlen(buf)+1);
    /* 此处不能关闭文件描述符，因为我们需要保持在进程中以读锁lock该文件*/
    return UGOK;
}

void pidfile_remove( const char *pidfile ) {
    xfiledel(pidfile);
}

int  pidfile_getpid( const char *pidfile,int *pid ) {
    FILE *fp = fopen(pidfile, "r");
    if (!fp) {
        /* fprintf(stderr, "fopen %s failed", pidfile); */
        return UGERR;
    }
    fscanf(fp, "%d", pid);
    fclose(fp);
    return UGOK;
}

int  pidfile_exists(const char *pidfile) {
    return (xfileisregular(pidfile) == UGERR) ;
}

int pidfile_verify(const char *pidfile) {
    int ret = UGERR;
    int fd = open(pidfile, O_RDWR, LOCKMODE);
    if (fd < 0) {
        return UGERR;
    }

    if (lock_file(fd) < 0) {
        //The  cmd argument is F_SETLK; the type of lock ( l_type) is a shared (F_RDLCK) or exclusive (F_WRLCK) lock and
        //the segment of a file to be locked is already exclusive-locked by another process, or the type is an exclusive
        //lock  and  some  portion of the segment of a file to be locked is already shared-locked or exclusive-locked by
        //another process.
        if (errno == EACCES || errno == EAGAIN) {
            ret = UGOK;
        }
    }
    close(fd);
    /* xfiledel(pidfile);  */ /* is danger */
    return ret;
}

int pkill(pid_t pid,int sig) {
    if (sig == -1) {
        sig = SIGTERM;
    }
    if (kill(pid, sig) < 0 ) {
        LOG_ERROR("kill [sig=%d] [pid=%d] failed",sig,pid);
        return(UGERR);
    }
    return UGOK;
}

int  proc_isrunning(pid_t pid, const char *matchstr) {
    char name[MAX_STRING_LEN]= {0};
    if (get_procname(pid, name, MAX_STRING_LEN) == UGOK &&
            xstrmatch(matchstr, name, 0)
       ) return UGOK;

    return UGERR;
}

