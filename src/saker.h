#ifndef _SAKER__H_
#define _SAKER__H_

#include <time.h>
#include "event/ae.h"
#include "lua.h"
#include "utils/ulist.h"
#include "utils/udict.h"
#include "config.h"

/* Client flags */
#define UG_SLAVE (1<<0)   /* This client is a slave server */
#define UG_MASTER (1<<1)  /* This client is a master server */

#define UG_MAX_WRITE_PER_EVENT (1024*64)

struct sakerServer
{
    int           hasfree;
    int           ipfd;
    int           isdaemon;
    int           iskiller;         
    dict         *commands;
    dict         *pubsub_channels;
    list         *pubsub_patterns;
    int           tcpkeepalive;               /* Set SO_KEEPALIVE if non-zero. */
    list         *clients;
    long long     stat_rejected_conn;
    long long     stat_numconnections;
    long long     max_timeeventid;
    aeEventLoop  *el;
     
    /* private */
    char         *pidfile;
    config_t     *config;
    char         *configfile;
    lua_State    *ls;
    dict         *tasks;
    dict         *process;
    
    /* not used */
    time_t        unixtime;
    unsigned long long maxmemory;
    unsigned lruclock:22;       /* Clock incrementing every minute, for LRU */
    unsigned lruclock_padding:10;
};

extern struct sakerServer server;

void initServer(struct sakerServer *server);

void freeServer(struct sakerServer *server);

#endif
