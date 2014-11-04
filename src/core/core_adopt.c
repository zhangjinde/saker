#include "core_declarer.h"
#include <assert.h>
#include "common/types.h"
#include "config.h"
#include "utils/string.h"
#include "utils/error.h"
#include "utils/file.h"
#include "utils/thread.h"
#include "utils/process.h"
#include "utils/sds.h"
#include "saker.h"
#ifndef OS_WIN
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif


sds getPidfile(const char *execfile) {
    sds   pidfile = sdsempty();
    char  buff[MAX_STRING_LEN] = {0};
    char *p, *q;
    strcat(buff, execfile);
    xstrlrtrim_spaces(buff);
    p = (p = strrchr(buff, ' ')) != NULL ? p+1 : buff ;
    if ((q = strrchr(p, '/')) != NULL) {
        q = q+1;
    } else if ((q=strrchr(p, '\\')) != NULL) {
        q = q+1;
    } else {
        q = p;
    }
    return sdscatprintf(pidfile, "%s/%s.pid" ,server.config->pidfile_dir, q);
}

int core_adopt(lua_State *L) {
    int ret = 2;
    int status = 0;
    sds cmd = sdsempty();
    sds pidfile = 0;
#ifdef OS_WIN
    PROCESS_INFORMATION processInfo;
    STARTUPINFO startupInfo;
    char        procname[1024]= {0};
#endif
    int idx = 1, argc = 0;
    pid_t pid;
    const char **argv = NULL;
    const char *user = NULL;
    sds *tmpargv = NULL;
    const char  *startcmd = NULL;
    int top = lua_gettop(L);
    if (top < 2) {
        lua_pushnil(L);
        lua_pushstring(L, "wrong number of arguments for fork");
        goto End;
    }

    startcmd = luaL_checkstring(L, 2);
    cmd = sdscat(cmd, startcmd);
    tmpargv = sdssplitargs(startcmd, &argc);

    argv = (const char **)zmalloc(sizeof(char *)*(argc+1));
    argv[argc] = 0;
    for (idx=0 ; idx<argc; ++idx) argv[idx] = tmpargv[idx];

    if (!lua_isnil(L, 1)) pidfile = sdsnew(luaL_checkstring(L, 1));
    else pidfile = getPidfile(argv[0]);

    if (top == 3) {
        user = luaL_checkstring(L, 3);
    }

    if ( pidfile_verify(pidfile) == UGOK) {
        lua_pushnil(L);
        lua_pushfstring(L, "the pidfile '%s' is exists", pidfile);
        goto End;
    }
#ifdef OS_WIN
    GetStartupInfo(&startupInfo); /* take defaults from current process */
    startupInfo.cb          = sizeof(STARTUPINFO);
    startupInfo.lpReserved  = NULL;
    startupInfo.lpDesktop   = NULL;
    startupInfo.lpTitle     = NULL;
    startupInfo.dwFlags     = STARTF_USESTDHANDLES;
    startupInfo.cbReserved2 = 0;
    startupInfo.lpReserved2 = NULL;

    if (!CreateProcess(
                NULL,
                cmd,
                NULL,
                NULL,
                FALSE,  // Do NOT inherit handles
                CREATE_NEW_PROCESS_GROUP | CREATE_UNICODE_ENVIRONMENT,
                NULL,
                NULL,
                &startupInfo,
                &processInfo
            )) {
        lua_pushnil(L);
        lua_pushfstring(L, "cannot fork process for  '%s'", argv[0]);
        goto End;
    }
    pid = processInfo.dwProcessId;
    if (WAIT_TIMEOUT != WaitForSingleObject(processInfo.hProcess, 10)) {
        GetExitCodeProcess(processInfo.hProcess, (DWORD *) &status);
        lua_pushnil(L);
        lua_pushfstring(L, "execute it [%s] failed ,status [%d] ,the pidfile :[%s]", cmd, status, pidfile);
        goto End;
    }

    get_procname(processInfo.hProcess, procname,1024);
    if (strlen(procname) != 0) {
        pidfile_create(pidfile, procname, pid);
    } else {
        LOG_ERROR("get process name failed '%'", cmd);
    }
#else
    pid = fork();
    if (pid < 0 ) {
        lua_pushnil(L);
        lua_pushfstring(L, "Cannot fork process for  '%s'", argv[0]);
        goto End;
    } else if (pid == 0) {
        int fd;
        int maxFd = (int)sysconf(_SC_OPEN_MAX);
        unsigned int flags = fcntl(server.ipfd, F_GETFD);
        int pid ;

        daemonize();

        /* Close all file descriptors, except for standard input,
                  standard output, standard error    and listen fd
               */
        for(fd = 3; fd < maxFd; ++fd) {
            if (fd != server.ipfd)
                close(fd);
        }

        /* FD_CLOEXEC, the close-on-exec flag.  If the FD_CLOEXEC bit is 0, the
                   file descriptor will remain open across an execve(2), otherwise it will be closed.
              */
        flags |= 1; // FD_CLOEXEC
        if(fcntl(server.ipfd, F_SETFD, flags) == -1) {
            close(server.ipfd);
            server.ipfd = -1;
        }
        pid = getpid();
        if (pidfile_create(pidfile, cmd, pid) == UGERR) {
            LOG_ERROR("cannot create pid for '%s' ,exit process. err: %s , mypid is '%d'", pidfile , xerrmsg(), pid);
            _exit(1);
        }
        if (user != NULL) {
            struct passwd *pwd = getpwnam(user);
            if (pwd) {
                if (setuid(pwd->pw_uid) != 0) {
                    LOG_ERROR("cannot setuid for '%s'", cmd);
                    _exit(1);
                }
            }
        }
        LOG_TRACE("exec %s", cmd);
        execvp(argv[0], (char * const *)argv);
        LOG_TRACE("exec process exit, mypid is '%d'", xerrmsg(), pid);
        xfiledel(pidfile);
        _exit(72);
    }
    /* avoid <defunct> Wait blocking */
    waitpid(pid, &status, 0);
    xsleep(10);
#endif
    if (pidfile_verify(pidfile) == UGOK) {
        lua_pushnumber(L, pid);
        lua_pushstring(L, pidfile);
    } else {
        lua_pushnil(L);
        lua_pushfstring(L, "child exited for '%s' ,please cat log by 'mypid is '%d''", pidfile, pid);
    }
End:
    if(tmpargv) sdsfreesplitres(tmpargv, argc);
    if(argv) zfree((char **)argv);
    argv = 0;
    sdsfree(cmd);
    if(pidfile) sdsfree(pidfile);
    return ret;
}

