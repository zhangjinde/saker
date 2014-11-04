#ifndef _CLIENT__H_
#define _CLIENT__H_


#include "event/ae.h"
#include "common/common.h"
#include "common/winfixes.h"
#include "utils/sds.h"
#include "utils/error.h"
#include "utils/ulist.h"
#include "utils/udict.h"
#include "event/anet.h"

#include "commands.h"

#define UG_SLAVE (1<<0)
#define UG_MASTER (1<<1)

#define UG_CLOSE_AFTER_REPLY (1<<6) /* Close after writing entire reply. */

#define UG_REPLY_CHUNK_BYTES (16*1024) /* 16k output buffer */
#define UG_INLINE_MAX_SIZE   (1024*64) /* Max size of inline reads */
#define UG_MBULK_BIG_ARG     (1024*32)

/* Client request types */
#define UG_REQ_INLINE 1
#define UG_REQ_MULTIBULK 2

struct protoObject;

typedef struct ugClient {
    int     fd;
    sds     querybuf;
    int     argc;
    struct protoObject  **argv;
    ugCommand* cmd;
    int     flags;
    list   *reply;
    unsigned long reply_bytes; /* Tot bytes of objects in reply list */
    int     multibulklen;
    int     bulklen;
    int     reqtype;
    time_t  ctime;           /* Client creation time */
    time_t  lastinteraction; /* time of the last interaction, used for timeout */
    dict *pubsub_channels;  /* channels a client is interested in (SUBSCRIBE) */
    list *pubsub_patterns;  /* patterns a client is interested in (SUBSCRIBE) */

    int bufpos;
    int sentlen;
    int authenticated;
    char buf[UG_REPLY_CHUNK_BYTES];
} ugClient;

size_t zmalloc_size_sds(sds s);
/* for networking */
void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask);

void readQueryFromClient(aeEventLoop *el, int fd, void *privdata, int mask) ;

void sendReplyToClient(aeEventLoop *el, int fd, void *privdata, int mask) ;

int prepareClientToWrite(ugClient *c);




/* for client */
ugClient* createClient(int fd);

void  freeClient(void *c);

void freeClientAsync(void *c);

void  resetClient(ugClient *c);

list* createClientlist();

void  freeClientlist(list *l);

int   processLineBuffer(ugClient* c);

int   processMultbuff(ugClient* c);

void  processInputBuffer(ugClient* c);

int   processCommand(ugClient* c);

//void  clearReply(ugClient* c);

/* reply */

void addReply(ugClient *c, struct protoObject *obj);

void addReplySds(ugClient *c, sds s);

void addReplyString(ugClient *c, char *s, size_t len) ;

void addReplyErrorLength(ugClient *c, char *s, size_t len);

void addReplyError(ugClient *c, char *err);

void addReplyErrorFormat(ugClient *c, const char *fmt, ...);

void addReplyStatusLength(ugClient *c, char *s, size_t len);

void addReplyStatus(ugClient *c, char *status);

void addReplyStatusFormat(ugClient *c, const char *fmt, ...) ;

void *addDeferredMultiBulkLength(ugClient *c) ;

void setDeferredMultiBulkLength(ugClient *c, void *node, long length) ;

void addReplyDouble(ugClient *c, double d);

void addReplyLongLongWithPrefix(ugClient *c, long long ll, char prefix);

void addReplyLongLong(ugClient *c, long long ll) ;

void addReplyMultiBulkLen(ugClient *c, long length);

void addReplyBulkLen(ugClient *c, struct protoObject *obj) ;

void addReplyBulk(ugClient *c, struct protoObject *obj);

void addReplyBulkCBuffer(ugClient *c, void *p, size_t len);

void addReplyBulkCString(ugClient *c, char *s);

void addReplyBulkLongLong(ugClient *c, long long ll);

#endif

