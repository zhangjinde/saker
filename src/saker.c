#include "saker.h"
#include <assert.h>
#include <signal.h>

#include "common/types.h"
#include "common/defines.h"
#include "luaworking.h"
#include "ulualib.h"

#include "utils/debug.h"
#include "utils/logger.h"
#include "utils/file.h"
#include "utils/path.h"
#include "utils/error.h"
#include "utils/getopt.h"
#include "utils/process.h"
#include "utils/xstring.h"
#include "utils/perf.h"

#include "service/register.h"
#include "sysinfo/sysinfo.h"
#include "sysinfo/top.h"

#include "event/anet.h"
#include "proto/client.h"
#include "proto/object.h"
#include "proto/pubsub.h"

extern const char *builtin_scripts[];

struct sakerServer server;

static void sigtermHandler(int sig) {
    UG_NOTUSED(sig);
    LOG_TRACE("Received SIGTERM, scheduling shutdown...");
    if (server.el) aeStop(server.el);
}

static void setupSignalHandler(void) {
    struct sigaction act;

    /* When the SA_SIGINFO flag is set in sa_flags then sa_sigaction is used.
     * Otherwise, sa_handler is used. */
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = sigtermHandler;
    sigaction(SIGTERM, &act, NULL);

#ifdef HAVE_BACKTRACE
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_NODEFER | SA_RESETHAND | SA_SIGINFO;
    act.sa_sigaction = sigsegvHandler;
    sigaction(SIGSEGV, &act, NULL);
    sigaction(SIGBUS, &act, NULL);
    sigaction(SIGFPE, &act, NULL);
    sigaction(SIGILL, &act, NULL);
#endif
}

static int  timerHandler(aeEventLoop *el, long long id, void *clientData) {
    dictIterator *di = dictGetIterator(server.tasks);
    dictEntry *de;
    const char *innererr = NULL;
    int ret = 1;
    char errmsg[LUAWORK_ERR_LEN] = {0};

    while ((de = dictNext(di)) != NULL) {
        ugTaskType *ptask = dictGetEntryVal(de);
        ret = 1;
        ugAssert(ptask != NULL);
        if (ptask->property == PROP_CYCLE || ptask->property == PROP_ONCE ) {
            if (luaworkCallByRef(server.ls, ptask->handle, errmsg, ">bs", &ret, &innererr) == UGERR) {
                LOG_ERROR("do task failed ,alias: '%s', err: '%s'", ptask->alias, errmsg);
                continue;
            }
            if (ret == 0) {
                LOG_ERROR("do task failed ,alias: '%s', err: '%s'", ptask->alias, innererr);
            }
        }
        if (ptask->property == PROP_ONCE) {
            ptask->property = PROP_PASSIVITY;
        }
    }
    dictReleaseIterator(di);
    LOG_TRACE("lua gc count after task  %d", lua_gc(server.ls, LUA_GCCOUNT, 0)*1024 + lua_gc(server.ls, LUA_GCCOUNTB, 0));
    LOG_TRACE("zmalloc used mem %d , rss:%d", zmalloc_used_memory(), zmalloc_get_rss()) ;

    return server.config->work_interval;
}


static int topHandler(aeEventLoop *el, long long id, void *clientData) {
    topUpdate();
    return server.config->top_mode;
}

static void deferHandler(void) {
    dictIterator *di = dictGetIterator(server.tasks);
    dictEntry *de;
    const char *innererr = NULL;
    char errmsg[LUAWORK_ERR_LEN] = {0};
    int ret;
    while ((de = dictNext(di)) != NULL) {
        ugTaskType *ptask =  dictGetEntryVal(de);
        assert(ptask != NULL);
        ret = 1;
        if (ptask->property == PROP_DEFER) {
            if (luaworkCallByRef(server.ls, ptask->handle, errmsg, ">bs", &ret, &innererr) == UGERR) {
                LOG_ERROR("do task failed ,alias: '%s', err: '%s'", ptask->alias, errmsg);
                continue;
            }
            if (ret == 0) {
                LOG_ERROR("do task failed ,alias: '%s', err: '%s'", ptask->alias, innererr);
            }
        }
    }
    dictReleaseIterator(di);
}

void initServer(struct sakerServer *server) {
    memset(server, 0 , sizeof(*server));
    server->hasfree = 0;

    server->ipfd = -1;
    server->isdaemon = UGERR;
    server->iskiller = UGERR;
    server->ls = NULL;
    server->el = NULL;
    server->tasks = NULL;
    server->process = createTop();
    server->tcpkeepalive = 60;
    server->clients = NULL;
    server->config = NULL;
    server->configfile = NULL;
    server->pidfile = NULL;

    server->commands = createCommands();
    server->pubsub_patterns = createPubsubPatterns();
    server->pubsub_channels = createPubsubChannels();
    createSharedObjects();
    server->maxmemory = 1;
    server->maxmemory = server->maxmemory < 30;

}

void freeServer(struct sakerServer *server) {
    int i;
    if (server->hasfree ) return;
    server->hasfree = 1;

    if (server->tasks) deferHandler();
    if (server->clients) freeClientlist(server->clients);
    if (server->ipfd > -1) aeDeleteFileEvent(server->el, server->ipfd, AE_READABLE);
    for (i=0; i <= server->max_timeeventid; ++i) {
        if (server->el)
            aeDeleteTimeEvent(server->el, i);
    }
    //if (server->timereventid != AE_ERR)
    if (server->el) aeDeleteEventLoop(server->el);
    if (server->config) freeConfig(server->config);
    if (server->tasks) destroyTaskMap(server->tasks);

    if (server->process) destroyTop(server->process);

    if (server->ls) luaworkClose(server->ls);
    if (server->pidfile && server->iskiller==UGOK) pidfile_remove(server->pidfile);
    if (server->pidfile) zfree(server->pidfile);

    if (server->commands) destroyCommands(server->commands);
    if (server->pubsub_channels) destroyPubsubChannels(server->pubsub_channels);
    if (server->pubsub_patterns) destroyPubsubPatterns(server->pubsub_patterns);
    destroySharedObjects();
}

void exitProc(void) {
    freeServer(&server);
    freeSysinfoDic();
    if (server.iskiller == UGERR) {
        LOG_INFO("Exit Server ...");
    }
    logger_close();
}

static int doAction(void) {
    char buff[MAX_STRING_LEN] = {0};
    int  idx = 0;
    long long  timeeventid;
    /* When the SA_SIGINFO flag is set in sa_flags then sa_sigaction is used.
     * Otherwise, sa_handler is used. */
    setupSignalHandler();

    atexit(exitProc);

    if ((server.config=createConfig(server.configfile)) == NULL) {
        LOG_ERROR("load_config failed");
        return UGERR;
    }

    if (UGERR == logger_open(server.config->logfile_path, server.config->logfile_level)) {
        LOG_ERROR("open_log failed");
        return UGERR;
    }
    server.pidfile = server.pidfile ? xstrdup(server.pidfile) : xstrprintf("%s/"APPNAME".pid", server.config->pidfile_dir);

    if (server.iskiller == UGOK) {
        pid_t pid ;
        if (UGERR == pidfile_getpid(server.pidfile, &pid)) {
            printf("cannot find running daemon process\n");
            exit(1);
        }
        pkill(pid, SIGTERM);
        printf("%d Killed\n", pid);
        exit(0);
    }
    if (server.isdaemon == UGOK) {
        if (UGOK == pidfile_verify(server.pidfile)) {
            LOG_WARNING(APPNAME" is running");
            printf(APPNAME" is alrunning\n");
            return UGERR;
        } else {
            pidfile_create(server.pidfile, APPNAME, getpid());
        }
    }

    server.ls = luaworkOpen();
    if (NULL == server.ls) {
        LOG_ERROR("luaworkOpen failed");
        return UGERR;
    }
    luaworkRefLib(server.ls, ulualib, buff);

    LOG_INFO("Enter Server");

    server.tasks = createTaskMap();
    if (NULL == server.tasks) {
        LOG_ERROR("init_monitor_container failed");
        return UGERR;
    }

    LOG_DEBUG("workspace(script) dir : %s", server.config->script_dir);
    for (idx=0; builtin_scripts[idx]; ++idx) {
        if (luaworkDoString(server.ls, builtin_scripts[idx], buff) != UGOK) {
            LOG_ERROR("do builtin_scripts to lua failed:%s ,err:%s",builtin_scripts[idx], buff);
            exit(1);
        }
    }

    if (luaworkDoDir(server.ls, server.config->script_dir, buff) != UGOK) {
        LOG_ERROR("lua_dodir [%s] failed,err:%s", server.config->script_dir, buff);
        exit(1);
    }

    server.config->maxclients = adjustOpenFilesLimit(server.config->maxclients);

    server.el = aeCreateEventLoop(server.config->maxclients + 1024);
    server.ipfd = anetTcpServer(buff, server.config->port, server.config->bind, 0);
    if (server.ipfd == ANET_ERR) {
        LOG_ERROR("bind localhost:%d failed . %s. ", server.config->port, xerrmsg());
        server.ipfd = 0;
        exit(1);
    }

    if (AE_ERR == aeCreateFileEvent(server.el, server.ipfd, AE_READABLE, acceptTcpHandler,NULL) ) {
        LOG_ERROR("aeCreateFileEvent failed");
        exit(1);
    }
    if ((timeeventid = aeCreateTimeEvent(server.el, 0, timerHandler,NULL,NULL)) == AE_ERR) {
        LOG_ERROR("aeCreateTimeEvent failed");
        exit(1);
    }
    server.max_timeeventid = timeeventid;

    if ((timeeventid = aeCreateTimeEvent(server.el, 0, topHandler, NULL, NULL)) == UGERR) {
        LOG_ERROR("aeCreateTimeEvent failed");
        exit(1);
    }
    server.max_timeeventid = timeeventid;
    aeMain(server.el);

    return UGOK;
}

void doBgAction(void) {
    daemonize();

    doAction();
}

static void doOptions(int argc, char **argv) {
    int idx;
    while ((idx = xgetopt(argc, argv, "kdc:p:")) != EOF)    {
        switch(idx) {
        case 'd':
            server.isdaemon = UGOK;
            break;
        case 'c':
            server.configfile = optarg;
            break;
        case 'k':
            server.iskiller = UGOK;
            break;
        case 'p':
            server.isdaemon = UGOK;
            server.pidfile = optarg;
            break;
        default:
            fprintf(stderr, APPNAME"\nUsage: %s [-kd] [-c configfile]\n",argv[0]);
            exit(1);
        }
    }
}

int main(int argc,char **argv) {
    initServer(&server);

    if (xchtoapppath() != UGOK) {
        fprintf(stderr, "chdir failed");
        exit(1);
    }

    doOptions(argc, argv);

    if (server.isdaemon == UGOK && server.iskiller == UGERR) {
        doBgAction();
    } else {
        doAction();
    }

    return UGOK;
}
