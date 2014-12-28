#include "client.h"
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "saker.h"
#include "object.h"
#include "pubsub.h"
#include "event/util.h"
#include "event/zmalloc.h"
#include "utils/string.h"
#include "utils/debug.h"
#include "utils/logger.h"
#include "utils/time.h"

void asyncCloseClientOnOutputBufferLimitReached(ugClient *c);

static void setProtocolError(ugClient *c, int pos);


/* Call() is the core of  execution of a command */
static void call(ugClient *c, int flags) {
    long long start = ustime(), duration;

    /* Call the command. */
    c->cmd->proc(c);

    duration = ustime()-start;

    c->cmd->microseconds += duration;
    c->cmd->calls++;
}

size_t zmalloc_size_sds(sds s) {
    return zmalloc_size(s-sizeof(struct sdshdr));
}

/* Functions managing dictionary of callbacks for pub/sub. */
static unsigned int callbackHash(const void *key) {
    robj *o = (robj *) key;

    if (o->encoding == UG_ENCODING_RAW) {
        return dictGenHashFunction(o->ptr, (int)sdslen((sds)o->ptr));
    } else {
        if (o->encoding == UG_ENCODING_INT) {
            char buf[32];
            int len;
            len = ll2string(buf, 32, (long)o->ptr);
            return dictGenHashFunction((unsigned char *)buf, len);
        } else {
            unsigned int hash;

            o = getDecodedObject(o);
            hash = dictGenHashFunction(o->ptr, (int)sdslen((sds)o->ptr));
            decrRefCount(o);
            return hash;
        }
    }
}

static int callbackKeyCompare(void *privdata, const void *key1, const void *key2) {
    robj *o1 = (robj *) key1, *o2 = (robj *) key2;

    return equalObjects(o1, o2);
}

static void callbackKeyDestructor(void *privdata, void *key) {
    DICT_NOTUSED(privdata);

    if (key == NULL) return; /* Values of swapped out keys as set to NULL */
    decrRefCount(key);
}



static dictType callbackDict = {
    callbackHash,
    NULL,
    NULL,
    callbackKeyCompare,
    callbackKeyDestructor,
    NULL
};

static int listMatchPubsubPattern(void *a, void *b) {
    sds s1 = (sds) a;
    sds s2 = (sds) b;
    return strcmp(s1, s2);
}

static void listFreePubsubPattern(void *ptr) {
    sds s = ptr;
    sdsfree(s);
}

static void listFreeReplyObjects(void *ptr) {
    decrRefCount(ptr);
}

static int  listMatchClientObjects(void *a, void *b) {
    ugClient *ca = (ugClient *) a;
    ugClient *cb = (ugClient *) b;
    return ca->fd == cb->fd;
}

static void listDeleteClientObjects(void *cl) {
    ugClient *c = (ugClient *) cl;
    resetClient(c);
    if (c->querybuf) sdsfree(c->querybuf);

    if (c->reply) {
        LOG_TRACE("reply size :%d", listLength(c->reply));
        listRelease(c->reply);
    }

    pubsubUnsubscribeAllChannels(c, 0);
    if (c->pubsub_channels) dictRelease(c->pubsub_channels);
    if (c->pubsub_patterns) listRelease(c->pubsub_patterns);

    aeDeleteFileEvent(server.el, c->fd, AE_READABLE);
    aeDeleteFileEvent(server.el, c->fd, AE_WRITABLE);
#ifdef _WIN32
    aeWinCloseSocket(c->fd);
#else
    close(c->fd);
#endif
    zfree(c);
    c = NULL;
}

sds getClientInfoString(ugClient *client) {
    sds s = sdsnew("clientinfo:");
    char ip[32];
    int port = 0;
    anetPeerToString(client->fd, ip, &port);
    s = sdscatprintf(s, "%s:%d" , ip, port);
    return s;
}

/* Helper function. Trims query buffer to make the function that processes
 * multi bulk requests idempotent. */
static void setProtocolError(ugClient *c, int pos) {
    sds client = getClientInfoString(c);
    LOG_ERROR("Protocol error from client: %s", client);
    sdsfree(client);
    c->flags |= UG_CLOSE_AFTER_REPLY;
    c->querybuf = sdsrange(c->querybuf,pos,-1);
}


static void freeClientArgv(ugClient *c) {
    int j;
    for (j = 0; j < c->argc; j++)
        decrRefCount(c->argv[j]);
    c->argc = 0;
}


ugClient *createClient(int fd) {
    ugClient *c = zmalloc(sizeof(ugClient));
    memset(c,0,sizeof(ugClient));
    if (fd != -1) {
        anetNonBlock(NULL,fd);
        anetEnableTcpNoDelay(NULL,fd);
        if (server.tcpkeepalive)
            anetKeepAlive(NULL, fd, server.tcpkeepalive);
        if (aeCreateFileEvent(server.el, fd, AE_READABLE,
                              readQueryFromClient, c) == AE_ERR) {
#ifdef _WIN32
            aeWinCloseSocket(fd);
#else
            close(fd);
#endif
            zfree(c);
            return NULL;
        }
    }

    if (server.config->password == NULL) {
        c->authenticated = 1;
    }
    c->querybuf = sdsempty();
    c->fd = fd;
    c->ctime = c->lastinteraction = time(NULL);
    c->multibulklen = 0;
    c->bulklen = -1;
    c->reply = listCreate();

    c->reply_bytes = 0;
    c->bufpos = 0;
    c->sentlen = 0;
    /* listSetDupMethod(c->reply, listDupReplyObjects); */
    listSetFreeMethod(c->reply, listFreeReplyObjects);

    c->pubsub_channels = dictCreate(&callbackDict, NULL);
    c->pubsub_patterns = listCreate();
    listSetMatchMethod(c->pubsub_patterns, listMatchPubsubPattern);
    listSetFreeMethod(c->pubsub_patterns, listFreePubsubPattern);
    /* c->pubsub_patterns = listCreate(); */
    if (!server.clients) {
        server.clients = createClientlist();
    }

    listAddNodeTail(server.clients, c);

    return c;
}

void  freeClient(void *vc) {
    ugClient *c = (ugClient *) vc;
    listNode *node = listSearchKey(server.clients, c);
    if (node) {
        listDelNode(server.clients, node);
    }

}

void freeClientAsync(void *c) {
    freeClient(c);
}

void  resetClient(ugClient *c) {
    if (c->argc) {
        freeClientArgv(c);
        zfree(c->argv);
        c->argv = NULL;
    }

    c->cmd = NULL;
    c->multibulklen = 0;
    c->reqtype = 0;
    c->bulklen = -1;
}

list *createClientlist( ) {
    list *clients = listCreate();
    listSetFreeMethod(clients, listDeleteClientObjects);
    listSetMatchMethod(clients, listMatchClientObjects);
    return clients;
}

void freeClientlist(list *l) {
    if(l) {
        listRelease(l);
    }
}

int  processLineBuffer(ugClient *c) {
    char *newline = strstr(c->querybuf, "\r\n");
    int argc, j;
    sds *argv;
    size_t querylen;

    /* Nothing to do without a \r\n */
    if (newline == NULL) {
        if (sdslen(c->querybuf) > UG_INLINE_MAX_SIZE) {
            addReplyErrorFormat(c, "Protocol error: too big inline request");
            setProtocolError(c,0);
        }
        return UGERR;
    }

    /* Split the input buffer up to the \r\n */
    querylen = newline-(c->querybuf);
    argv = sdssplitlen(c->querybuf,(int)querylen," ",1,&argc);

    /* Leave data after the first line of the query in the buffer */
    c->querybuf = sdsrange(c->querybuf,(int)(querylen+2),-1);

    /* Setup argv array on client structure */
    if (c->argv) zfree(c->argv);
    c->argv = zmalloc(sizeof(robj *)*argc);

    /* Create redis objects for all arguments. */
    for (c->argc = 0, j = 0; j < argc; j++) {
        if (sdslen(argv[j])) {
            c->argv[c->argc] = createObject(UG_STRING,argv[j]);
            c->argc++;
        } else {
            sdsfree(argv[j]);
        }
    }
    zfree(argv);
    return UGOK;
}

int  processMultbuff(ugClient *c) {
    char *newline = NULL;
    int pos = 0, ok;
    long long ll;

    if (c->multibulklen == 0) {
        /* The client should have been reset */
        ugAssert(c->argc == 0);

        /* Multi bulk length cannot be read without a \r\n */
        newline = strchr(c->querybuf,'\r');
        if (newline == NULL) {
            if (sdslen(c->querybuf) > UG_INLINE_MAX_SIZE) {
                addReplyErrorFormat(c,"Protocol error: too big mbulk count string");
                setProtocolError(c,0);
            }
            return UGERR;
        }

        /* Buffer should also contain \n */
        if (newline-(c->querybuf) > ((signed)sdslen(c->querybuf)-2))
            return UGERR;

        /* We know for sure there is a whole line since newline != NULL,
         * so go ahead and find out the multi bulk length. */
        ugAssert(c->querybuf[0] == '*');
        ok = string2ll(c->querybuf+1,newline-(c->querybuf+1),&ll);
        if (!ok || ll > 1024*1024) {
            addReplyErrorFormat(c,"Protocol error: invalid multibulk length");
            setProtocolError(c, pos);
            return UGERR;
        }

        pos = (int)(newline-c->querybuf)+2;
        if (ll <= 0) {
            c->querybuf = sdsrange(c->querybuf,pos,-1);
            return UGOK;
        }

        c->multibulklen = (int)ll;

        /* Setup argv array on client structure */
        if (c->argv) zfree(c->argv);
        c->argv = zmalloc(sizeof(robj *)*c->multibulklen);
    }

    ugAssert(c->multibulklen > 0);
    while(c->multibulklen) {
        /* Read bulk length if unknown */
        if (c->bulklen == -1) {
            newline = strchr(c->querybuf+pos,'\r');
            if (newline == NULL) {
                if (sdslen(c->querybuf) > UG_INLINE_MAX_SIZE) {
                    addReplyErrorFormat(c,"Protocol error: too big bulk count string");
                    setProtocolError(c,0);
                }
                break;
            }

            /* Buffer should also contain \n */
            if (newline-(c->querybuf) > ((signed)sdslen(c->querybuf)-2))
                break;

            if (c->querybuf[pos] != '$') {
                addReplyErrorFormat(c,
                                    "Protocol error: expected '$', got '%c'",
                                    c->querybuf[pos]);
                setProtocolError(c,pos);
                return UGERR;
            }

            ok = string2ll(c->querybuf+pos+1,newline-(c->querybuf+pos+1),&ll);
            if (!ok || ll < 0 || ll > 512*1024*1024) {
                addReplyErrorFormat(c, "Protocol error: invalid bulk length");
                setProtocolError(c, pos);
                return UGERR;
            }

            pos += (int)(newline-(c->querybuf+pos)+2);
            if (ll >= UG_MBULK_BIG_ARG) {
                /* If we are going to read a large object from network
                 * try to make it likely that it will start at c->querybuf
                 * boundary so that we can optimized object creation
                 * avoiding a large copy of data. */
                c->querybuf = sdsrange(c->querybuf,pos,-1);
                pos = 0;
                /* Hint the sds library about the amount of bytes this string is
                 * going to contain. */
                c->querybuf = sdsMakeRoomFor(c->querybuf,(size_t)(ll+2));
            }
            c->bulklen = (long)ll;
        }

        /* Read bulk argument */
        if (sdslen(c->querybuf)-pos < (unsigned)(c->bulklen+2)) {
            /* Not enough data (+2 == trailing \r\n) */
            break;
        } else {
            /* Optimization: if the buffer contains JUST our bulk element
             * instead of creating a new object by *copying* the sds we
             * just use the current sds string. */
            if (pos == 0 &&
                    c->bulklen >= UG_MBULK_BIG_ARG &&
                    (signed) sdslen(c->querybuf) == c->bulklen+2) {
                c->argv[c->argc++] = createObject(UG_STRING, c->querybuf);
                sdsIncrLen(c->querybuf,-2); /* remove CRLF */
                c->querybuf = sdsempty();
                /* Assume that if we saw a fat argument we'll see another one
                 * likely... */
                c->querybuf = sdsMakeRoomFor(c->querybuf,c->bulklen+2);
                pos = 0;
            } else {
                c->argv[c->argc++] =
                    createStringObject(c->querybuf+pos, c->bulklen);
                pos += c->bulklen+2;
            }
            c->bulklen = -1;
            c->multibulklen--;
        }
    }

    /* Trim to pos */
    if (pos) c->querybuf = sdsrange(c->querybuf,pos,-1);

    /* We're done when c->multibulk == 0 */
    if (c->multibulklen == 0) return UGOK;

    /* Still not read to process the command */
    return UGERR;
}

void  processInputBuffer(ugClient *c) {
    while(sdslen(c->querybuf)) {
        if (c->flags & UG_CLOSE_AFTER_REPLY) return;

        /* Determine request type when unknown. */
        if (!c->reqtype) {
            if (c->querybuf[0] == '*') {
                c->reqtype = UG_REQ_MULTIBULK;
            } else {
                c->reqtype = UG_REQ_INLINE;
            }
        }

        if (c->reqtype == UG_REQ_INLINE) {
            if (processLineBuffer(c) != UGOK) break;
        } else if (c->reqtype == UG_REQ_MULTIBULK) {
            if (processMultbuff(c) != UGOK) break;
        } else {
            ugPanic("Unknown request type");
        }

        if (c->argc == 0) {
            resetClient(c);
        } else {
            if (processCommand(c) == UGOK) {
                resetClient(c);
            }
        }

    }
}


int  processCommand(ugClient *c) {
    if (!strcasecmp(c->argv[0]->ptr,"quit")) {
        addReply(c,shared.ok);
        c->flags |= UG_CLOSE_AFTER_REPLY;
        return UGERR;
    }
    if (c->authenticated  == 0 && strcasecmp(c->argv[0]->ptr, "auth") != 0) {
        addReplyError(c,"NOAUTH Authentication required. ");
        return UGOK;
    }
    c->cmd = lookupCommand((sds)c->argv[0]->ptr)  ;
    if (!c->cmd) {
        addReplyErrorFormat(c,"unknown command '%s'",
                            (char *)c->argv[0]->ptr);
        return UGOK;
    }  else if (c->cmd->params != -1 && c->argc-1 != c->cmd->params) {
        addReplyErrorFormat(c,"wrong number of arguments for '%s' command",
                            c->cmd->name);
        return UGOK;
    }
    call(c, 0);
    /* must has reply */
    ugAssert(strlen(c->buf) || listLength(c->reply));
    //if (addReply(c, ) == UGERR) {
    //    c->flags |= UG_CLOSE_AFTER_REPLY;
    //    return UGERR;
    //}


    return UGOK;
}

/* Create a duplicate of the last object in the reply list when
 * it is not exclusively owned by the reply list. */
robj *dupLastObjectIfNeeded(list *reply) {
    robj *new, *cur;
    listNode *ln;
    ugAssert(listLength(reply) > 0);
    ln = listLast(reply);
    cur = listNodeValue(ln);
    if (cur->refcount > 1) {
        new = dupStringObject(cur);
        decrRefCount(cur);
        listNodeValue(ln) = new;
    }
    return listNodeValue(ln);
}

/* -----------------------------------------------------------------------------
 * Low level functions to add more data to output buffers.
 * -------------------------------------------------------------------------- */

int _addReplyToBuffer(ugClient *c, char *s, size_t len) {
    size_t available = sizeof(c->buf)-c->bufpos;

    if (c->flags & UG_CLOSE_AFTER_REPLY) return UGOK;

    /* If there already are entries in the reply list, we cannot
     * add anything more to the static buffer. */
    if (listLength(c->reply) > 0) return UGERR;

    /* Check that the buffer has enough space available for this string. */
    if (len > available) return UGERR;

    memcpy(c->buf+c->bufpos,s,len);
    c->bufpos+=(int)len;
    return UGOK;
}

void _addReplyObjectToList(ugClient *c, robj *o) {
    robj *tail;

    if (c->flags & UG_CLOSE_AFTER_REPLY) return;

    if (listLength(c->reply) == 0) {
        incrRefCount(o);
        listAddNodeTail(c->reply,o);
        c->reply_bytes += (unsigned long)zmalloc_size_sds(o->ptr);
    } else {
        tail = listNodeValue(listLast(c->reply));

        /* Append to this object when possible. */
        if (tail->ptr != NULL &&
                sdslen(tail->ptr)+sdslen(o->ptr) <= UG_REPLY_CHUNK_BYTES) {
            c->reply_bytes -= (unsigned long)zmalloc_size_sds(tail->ptr);
            tail = dupLastObjectIfNeeded(c->reply);
            tail->ptr = sdscatlen(tail->ptr,o->ptr,sdslen(o->ptr));
            c->reply_bytes += (unsigned long)zmalloc_size_sds(tail->ptr);
        } else {
            incrRefCount(o);
            listAddNodeTail(c->reply,o);
            c->reply_bytes += (unsigned long)zmalloc_size_sds(o->ptr);
        }
    }
    asyncCloseClientOnOutputBufferLimitReached(c);
}

/* This method takes responsibility over the sds. When it is no longer
 * needed it will be free'd, otherwise it ends up in a robj. */
void _addReplySdsToList(ugClient *c, sds s) {
    robj *tail;

    if (c->flags & UG_CLOSE_AFTER_REPLY) {
        sdsfree(s);
        return;
    }

    if (listLength(c->reply) == 0) {
        listAddNodeTail(c->reply,createObject(UG_STRING,s));
        c->reply_bytes += (unsigned long)zmalloc_size_sds(s);
    } else {
        tail = listNodeValue(listLast(c->reply));

        /* Append to this object when possible. */
        if (tail->ptr != NULL &&
                sdslen(tail->ptr)+sdslen(s) <= UG_REPLY_CHUNK_BYTES) {
            c->reply_bytes -= (unsigned long)zmalloc_size_sds(tail->ptr);
            tail = dupLastObjectIfNeeded(c->reply);
            tail->ptr = sdscatlen(tail->ptr,s,sdslen(s));
            c->reply_bytes += (unsigned long)zmalloc_size_sds(tail->ptr);
            sdsfree(s);
        } else {
            listAddNodeTail(c->reply,createObject(UG_STRING,s));
            c->reply_bytes += (unsigned long)zmalloc_size_sds(s);
        }
    }
    asyncCloseClientOnOutputBufferLimitReached(c);
}

void _addReplyStringToList(ugClient *c, char *s, size_t len) {
    robj *tail;

    if (c->flags & UG_CLOSE_AFTER_REPLY) return;

    if (listLength(c->reply) == 0) {
        robj *o = createStringObject(s,len);

        listAddNodeTail(c->reply,o);
        c->reply_bytes += (unsigned long)zmalloc_size_sds(o->ptr);
    } else {
        tail = listNodeValue(listLast(c->reply));

        /* Append to this object when possible. */
        if (tail->ptr != NULL &&
                sdslen(tail->ptr)+len <= UG_REPLY_CHUNK_BYTES) {
            c->reply_bytes -= (unsigned long)zmalloc_size_sds(tail->ptr);
            tail = dupLastObjectIfNeeded(c->reply);
            tail->ptr = sdscatlen(tail->ptr,s,len);
            c->reply_bytes += (unsigned long)zmalloc_size_sds(tail->ptr);
        } else {
            robj *o = createStringObject(s,len);

            listAddNodeTail(c->reply,o);
            c->reply_bytes += (unsigned long)zmalloc_size_sds(o->ptr);
        }
    }
    asyncCloseClientOnOutputBufferLimitReached(c);
}

/* -----------------------------------------------------------------------------
 * Higher level functions to queue data on the client output buffer.
 * The following functions are the ones that commands implementations will call.
 * -------------------------------------------------------------------------- */

void addReply(ugClient *c, robj *obj) {
    if (prepareClientToWrite(c) != UGOK) return;

    /* This is an important place where we can avoid copy-on-write
     * when there is a saving child running, avoiding touching the
     * refcount field of the object if it's not needed.
     *
     * If the encoding is RAW and there is room in the static buffer
     * we'll be able to send the object to the client without
     * messing with its page. */
    if (obj->encoding == UG_ENCODING_RAW) {
        if (_addReplyToBuffer(c,obj->ptr,sdslen(obj->ptr)) != UGOK)
            _addReplyObjectToList(c,obj);
    } else if (obj->encoding == UG_ENCODING_INT) {
        /* Optimization: if there is room in the static buffer for 32 bytes
         * (more than the max chars a 64 bit integer can take as string) we
         * avoid decoding the object and go for the lower level approach. */
        if (listLength(c->reply) == 0 && (sizeof(c->buf) - c->bufpos) >= 32) {
            char buf[32];
            int len;

            len = ll2string(buf,sizeof(buf),(long)obj->ptr);
            if (_addReplyToBuffer(c,buf,len) == UGOK)
                return;
            /* else... continue with the normal code path, but should never
             * happen actually since we verified there is room. */
        }
        obj = getDecodedObject(obj);
        if (_addReplyToBuffer(c,obj->ptr,sdslen(obj->ptr)) != UGOK)
            _addReplyObjectToList(c,obj);
        decrRefCount(obj);
    } else {
        ugPanic("Wrong obj->encoding in addReply()");
    }
}

void addReplySds(ugClient *c, sds s) {
    if (prepareClientToWrite(c) != UGOK) {
        /* The caller expects the sds to be free'd. */
        sdsfree(s);
        return;
    }
    if (_addReplyToBuffer(c,s,sdslen(s)) == UGOK) {
        sdsfree(s);
    } else {
        /* This method free's the sds when it is no longer needed. */
        _addReplySdsToList(c,s);
    }
}

void addReplyString(ugClient *c, char *s, size_t len) {
    if (prepareClientToWrite(c) != UGOK) return;
    if (_addReplyToBuffer(c,s,len) != UGOK)
        _addReplyStringToList(c,s,len);
}

void addReplyErrorLength(ugClient *c, char *s, size_t len) {
    addReplyString(c,"-ERR ",5);
    addReplyString(c,s,len);
    addReplyString(c,"\r\n",2);
}

void addReplyError(ugClient *c, char *err) {
    addReplyErrorLength(c,err,strlen(err));
}

void addReplyErrorFormat(ugClient *c, const char *fmt, ...) {
    size_t l, j;
    va_list ap;
    sds s;
    va_start(ap,fmt);
    s = sdscatvprintf(sdsempty(),fmt,ap);
    va_end(ap);
    /* Make sure there are no newlines in the string, otherwise invalid protocol
     * is emitted. */
    l = sdslen(s);
    for (j = 0; j < l; j++) {
        if (s[j] == '\r' || s[j] == '\n') s[j] = ' ';
    }
    addReplyErrorLength(c,s,sdslen(s));
    sdsfree(s);
}

void addReplyStatusLength(ugClient *c, char *s, size_t len) {
    addReplyString(c,"+",1);
    addReplyString(c,s,len);
    addReplyString(c,"\r\n",2);
}

void addReplyStatus(ugClient *c, char *status) {
    addReplyStatusLength(c,status,strlen(status));
}

void addReplyStatusFormat(ugClient *c, const char *fmt, ...) {
    sds s;
    va_list ap;
    va_start(ap,fmt);
    s = sdscatvprintf(sdsempty(),fmt,ap);
    va_end(ap);
    addReplyStatusLength(c,s,sdslen(s));
    sdsfree(s);
}

/* Adds an empty object to the reply list that will contain the multi bulk
 * length, which is not known when this function is called. */
void *addDeferredMultiBulkLength(ugClient *c) {
    /* Note that we install the write event here even if the object is not
     * ready to be sent, since we are sure that before returning to the
     * event loop setDeferredMultiBulkLength() will be called. */
    if (prepareClientToWrite(c) != UGOK) return NULL;
    listAddNodeTail(c->reply,createObject(UG_STRING,NULL));
    return listLast(c->reply);
}

/* Populate the length object and try gluing it to the next chunk. */
void setDeferredMultiBulkLength(ugClient *c, void *node, long length) {
    listNode *ln = (listNode *)node;
    robj *len, *next;

    /* Abort when *node is NULL (see addDeferredMultiBulkLength). */
    if (node == NULL) return;

    len = listNodeValue(ln);
    len->ptr = sdscatprintf(sdsempty(),"*%ld\r\n",length);
    c->reply_bytes += (unsigned long)zmalloc_size_sds(len->ptr);
    if (ln->next != NULL) {
        next = listNodeValue(ln->next);

        /* Only glue when the next node is non-NULL (an sds in this case) */
        if (next->ptr != NULL) {
            c->reply_bytes -= (unsigned long)zmalloc_size_sds(len->ptr);
            c->reply_bytes -= (unsigned long)zmalloc_size_sds(next->ptr);
            len->ptr = sdscatlen(len->ptr,next->ptr,sdslen(next->ptr));
            c->reply_bytes += (unsigned long)zmalloc_size_sds(len->ptr);
            listDelNode(c->reply,ln->next);
        }
    }
    asyncCloseClientOnOutputBufferLimitReached(c);
}

/* Add a double as a bulk reply */
void addReplyDouble(ugClient *c, double d) {
    char dbuf[128], sbuf[128];
    int dlen, slen;
#ifdef _WIN32
    if (isnan(d)) {
        dlen = snprintf(dbuf,sizeof(dbuf),"nan");
    } else if (isinf(d)) {
        if (d < 0)
            dlen = snprintf(dbuf,sizeof(dbuf),"-inf");
        else
            dlen = snprintf(dbuf,sizeof(dbuf),"inf");
    } else {
        dlen = snprintf(dbuf,sizeof(dbuf),"%.17g",d);
    }
#else
    dlen = snprintf(dbuf,sizeof(dbuf),"%.17g",d);
#endif
    slen = snprintf(sbuf,sizeof(sbuf),"$%d\r\n%s\r\n",dlen,dbuf);
    addReplyString(c,sbuf,slen);
}

/* Add a long long as integer reply or bulk len / multi bulk count.
 * Basically this is used to output <prefix><long long><crlf>. */
void addReplyLongLongWithPrefix(ugClient *c, long long ll, char prefix) {
    char buf[128];
    int len;

    /* Things like $3\r\n or *2\r\n are emitted very often by the protocol
     * so we have a few shared objects to use if the integer is small
     * like it is most of the times. */
    if (prefix == '*' && ll < UG_SHARED_BULKHDR_LEN) {
        addReply(c,shared.mbulkhdr[ll]);
        return;
    } else if (prefix == '$' && ll < UG_SHARED_BULKHDR_LEN) {
        addReply(c,shared.bulkhdr[ll]);
        return;
    }

    buf[0] = prefix;
    len = ll2string(buf+1,sizeof(buf)-1,ll);
    buf[len+1] = '\r';
    buf[len+2] = '\n';
    addReplyString(c,buf,len+3);
}

void addReplyLongLong(ugClient *c, long long ll) {
    if (ll == 0)
        addReply(c,shared.czero);
    else if (ll == 1)
        addReply(c,shared.cone);
    else
        addReplyLongLongWithPrefix(c,ll,':');
}

void addReplyMultiBulkLen(ugClient *c, long length) {
    addReplyLongLongWithPrefix(c,length,'*');
}

/* Create the length prefix of a bulk reply, example: $2234 */
void addReplyBulkLen(ugClient *c, robj *obj) {
    size_t len;

    if (obj->encoding == UG_ENCODING_RAW) {
        len = sdslen(obj->ptr);
    } else {
        long n = (long)obj->ptr;

        /* Compute how many bytes will take this integer as a radix 10 string */
        len = 1;
        if (n < 0) {
            len++;
            n = -n;
        }
        while((n = n/10) != 0) {
            len++;
        }
    }
    addReplyLongLongWithPrefix(c,len,'$');
}

/* Add a Redis Object as a bulk reply */
void addReplyBulk(ugClient *c, robj *obj) {
    addReplyBulkLen(c,obj);
    addReply(c,obj);
    addReply(c,shared.crlf);
}

/* Add a C buffer as bulk reply */
void addReplyBulkCBuffer(ugClient *c, void *p, size_t len) {
    addReplyLongLongWithPrefix(c,len,'$');
    addReplyString(c,p,len);
    addReply(c,shared.crlf);
}

/* Add a C nul term string as bulk reply */
void addReplyBulkCString(ugClient *c, char *s) {
    if (s == NULL) {
        addReply(c,shared.nullbulk);
    } else {
        addReplyBulkCBuffer(c,s,strlen(s));
    }
}

/* Add a long long as a bulk reply */
void addReplyBulkLongLong(ugClient *c, long long ll) {
    char buf[64];
    int len;

    len = ll2string(buf,64,ll);
    addReplyBulkCBuffer(c,buf,len);
}

/* Asynchronously close a client if soft or hard limit is reached on the
 * output buffer size. The caller can check if the client will be closed
 * checking if the client REDIS_CLOSE_ASAP flag is set.
 *
 * Note: we need to close the client asynchronously because this function is
 * called from contexts where the client can't be freed safely, i.e. from the
 * lower level functions pushing data inside the client output buffers. */
void asyncCloseClientOnOutputBufferLimitReached(ugClient *c) {
    /*
    ugAssert(c->reply_bytes < ULONG_MAX-(1024*64));
    if (c->reply_bytes == 0 || c->flags & REDIS_CLOSE_ASAP) return;
    if (checkClientOutputBufferLimits(c)) {
        sds client = getClientInfoString(c);

        freeClientAsync(c);
        redisLog(REDIS_WARNING,"Client %s scheduled to be closed ASAP for overcoming of output buffer limits.", client);
        sdsfree(client);
    }
    */
}
