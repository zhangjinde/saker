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
#endif

int core_exec(lua_State* L)
{
#ifdef OS_WIN
    PROCESS_INFORMATION processInfo;
    STARTUPINFO startupInfo;
    char procname[1024]= {0};
#else
    time_t starttime;
#endif
    int timeout = 10; /* default 10 ms */
    int ret = 2;
    int status = 0;
    sds cmd = sdsempty();
    sds pidfile = 0;
    int idx = 1, argc = 0 ,rc;
    pid_t pid;
    const char** argv = NULL;
    sds* tmpargv = NULL;
    const char*  startcmd = NULL;
    int top = lua_gettop(L);
    if (top < 1) {
        lua_pushnil(L);
        lua_pushstring(L, "wrong number of arguments for fork");
        goto End;
    }

    startcmd = lua_tostring(L, 1);
    if (top > 1) timeout = lua_tointeger(L, 2);
    cmd = sdscat(cmd, startcmd);
    tmpargv = sdssplitargs(startcmd, &argc);

    argv = (const char**)zmalloc(sizeof(char*)*(argc+1));
    argv[argc] = 0;
    for (idx=0 ; idx<argc; ++idx) argv[idx] = tmpargv[idx];

#ifdef OS_WIN
    UG_NOTUSED(rc);
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

    if (WAIT_TIMEOUT == WaitForSingleObject(processInfo.hProcess, timeout)) {
        lua_pushnil(L);
        lua_pushfstring(L, "WaitForSingleObject  '%s' timeout.", startcmd);
        pkill(pid, -1);
        goto End;
    }
    /*  give the retval to user     */
    GetExitCodeProcess(processInfo.hProcess, (DWORD*) &status);

#else
    alarm(timeout);

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
        LOG_TRACE("exec %s", cmd);
        execvp(argv[0], argv);
        _exit(0);
    }
    /* Wait timeout */
    starttime = time(NULL);
    do {
        if (-1 == (rc = waitpid(pid, &status, WUNTRACED)) && EINTR != xerrno()) {
            alarm(0);
            lua_pushnil(L);
            lua_pushfstring(L, "waitpid failure: '%s' ,err: '%s'", startcmd, xerrmsg());
            goto End;
        } else if (rc == -1 ) {
            alarm(0);
            lua_pushnil(L);
            lua_pushfstring(L, "waitpid timeout: '%s' ,err: '%s'", startcmd, xerrmsg());
            goto End;
        }
        if (WIFEXITED(status)) {
            LOG_TRACE( "%s exited, status:%d", startcmd, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            LOG_TRACE( "%s killed by signal %d", startcmd, WTERMSIG(status));
        } else if (WIFSTOPPED(status)) {
            LOG_TRACE( "%s stopped by signal %d", startcmd, WSTOPSIG(status));
        }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    alarm(0);
#endif
    lua_pushinteger(L, status);
    lua_pushnil(L);
End:
    if(tmpargv) sdsfreesplitres(tmpargv, argc);
    if(argv) zfree((char**)argv);
    argv = 0;
    sdsfree(cmd);
    if(pidfile) sdsfree(pidfile);
    return ret;
}

