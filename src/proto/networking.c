
#include "client.h"
#include <time.h>
#include "common/common.h"
#include "proto/object.h"
#include "saker.h"
#include "utils/logger.h"
#include "utils/error.h"



static void acceptCommonhandler(int fd, int flags)
{
    ugClient* c;
    if ((c = createClient(fd)) == NULL) {
        LOG_TRACE(
            "Error registering fd event for the new client: %s (fd=%d)",
            xerrmsg(),fd);
#ifdef _WIN32
        aeWinCloseSocket(fd); /* May be already closed, just ingore errors */
#else
        close(fd); /* May be already closed, just ignore errors */
#endif
        return;
    }
	    /* If maxclient directive is set and this is one client more... close the
     * connection. Note that we create the client instead to check before
     * for this condition, since now the socket is already set in non-blocking
     * mode and we can send an error for free using the Kernel I/O */
    if ((unsigned long long)listLength(server.clients) > server.config->maxclients) {
        char *err = "-ERR max number of clients reached\r\n";

        /* That's a best effort error message, don't check write errors */
#ifdef _WIN32
        if (send((SOCKET)c->fd,err,(int)strlen(err),0) == SOCKET_ERROR) {
#else
        if (write(c->fd,err,strlen(err)) == -1) {
#endif
            /* Nothing to do, Just to avoid the warning... */
        }
        server.stat_rejected_conn++;
        freeClient(c);
        return;
    }
    server.stat_numconnections++;
    c->flags |= flags;
}

void readQueryFromClient(aeEventLoop* el, int fd, void* privdata, int mask)
{
    ugClient* c = (ugClient*) privdata;
    int nread, readlen;
    size_t qblen;
    UG_NOTUSED(el);
    UG_NOTUSED(mask);
    readlen = 1024*16;

    /* If this is a multi bulk request, and we are processing a bulk reply
    * that is large enough, try to maximize the probability that the query
    * buffer contains exactly the SDS string representing the object, even
    * at the risk of requiring more read(2) calls. This way the function
    * processMultiBulkBuffer() can avoid copying buffers to create the
    * Redis Object representing the argument. */
    if (c->reqtype == UG_REQ_MULTIBULK && c->multibulklen && c->bulklen != -1
            && c->bulklen >= UG_MBULK_BIG_ARG) {
        int remaining = (int)((unsigned)(c->bulklen+2)-sdslen(c->querybuf));

        if (remaining < readlen) readlen = remaining;
    }

    qblen = sdslen(c->querybuf);
    if (sdsavail(c->querybuf) < (size_t) readlen) {
        c->querybuf = sdsMakeRoomFor(c->querybuf, readlen-sdsavail(c->querybuf));
    }

#ifdef _WIN32
    nread = recv((SOCKET)fd, c->querybuf+qblen, readlen, 0);
    if (nread < 0) {
        errno = WSAGetLastError();
        if (errno == WSAECONNRESET) {
            /* Windows fix: Not an error, intercept it.  */
            LOG_TRACE("Client closed connection");
            freeClient(c);
            return;
        } else if ((errno == ENOENT) || (errno == WSAEWOULDBLOCK)) {
            /* Windows fix: Intercept winsock slang for EAGAIN */
            errno = EAGAIN;
            nread = -1; /* Winsock can send ENOENT instead EAGAIN */
        }
    }
#else
    nread = read(fd, c->querybuf+qblen, readlen);
#endif
    if (nread == -1) {
        if (errno == EAGAIN) {
            nread = 0;
        } else {
            LOG_TRACE("Reading from client: %s", xerrmsg());
            freeClient(c);
            return;
        }
    } else if (nread == 0) {
        LOG_TRACE("Client closed connection");
        freeClient(c);
        return;
    }
#ifdef _WIN32
    aeWinReceiveDone(fd);
#endif
    if (nread) {
        sdsIncrLen(c->querybuf, nread);
        c->lastinteraction = time(NULL);
    } else {
        return;
    }

    LOG_TRACE("read buff from client : %s", c->querybuf);
    processInputBuffer(c);
}




void acceptTcpHandler(aeEventLoop* el, int fd, void* privdata, int mask)
{
    int cport, cfd;
    char cip[128];
    char errbuff[MAX_STRING_LEN] = {0};
    UG_NOTUSED(el);
    UG_NOTUSED(mask);
    UG_NOTUSED(privdata);

    cfd = anetTcpAccept(errbuff, fd, cip, &cport);
    if (cfd == AE_ERR) {
        LOG_WARNING("Accepting client connection: %s", errbuff);
        return;
    }
    LOG_TRACE("Accepted %s:%d", cip, cport);
    acceptCommonhandler(cfd, 0);
}



#ifdef _WIN32
void sendReplyBufferDone(aeEventLoop *el, int fd, void *privdata, int written) {
    aeWinSendReq *req = (aeWinSendReq *)privdata;
    ugClient *c = (ugClient *)req->client;
    int offset = (int)(req->buf - (char *)req->data + written);
    UG_NOTUSED(el);
    UG_NOTUSED(fd);

    if (c->bufpos == offset) {
        c->bufpos = 0;
        c->sentlen = 0;
    }
    if (c->bufpos == 0 && listLength(c->reply) == 0) {
        aeDeleteFileEvent(server.el,c->fd,AE_WRITABLE);

        /* Close connection after entire reply has been sent. */
        if (c->flags & UG_CLOSE_AFTER_REPLY) {
            freeClientAsync(c);
        }
    }
}

void sendReplyListDone(aeEventLoop *el, int fd, void *privdata, int written) {
    aeWinSendReq *req = (aeWinSendReq *)privdata;
    ugClient *c = (ugClient *)req->client;
    robj *o = (robj *)req->data;
    UG_NOTUSED(el);
    UG_NOTUSED(fd);

    decrRefCount(o);

    if (c->bufpos == 0 && listLength(c->reply) == 0) {
        c->sentlen = 0;
        aeDeleteFileEvent(server.el,c->fd,AE_WRITABLE);

        /* Close connection after entire reply has been sent. */
        if (c->flags & UG_CLOSE_AFTER_REPLY){
            freeClientAsync(c);
        }
    }
}

void sendReplyToClient(aeEventLoop *el, int fd, void *privdata, int mask) {
    ugClient *c = (ugClient *)privdata;
    int nwritten = 0, totwritten = 0, objlen;
    size_t objmem;
    robj *o;
    int result = 0;
    listIter li;
    listNode *ln;
    UG_NOTUSED(el);
    UG_NOTUSED(mask);

    if (c->flags & UG_MASTER) {
        /* do not send to master */
        c->bufpos = 0;
        c->sentlen = 0;
        while (listLength(c->reply)) {
            listDelNode(c->reply,listFirst(c->reply));
        }
        c->lastinteraction = time(NULL);
        return;
    }

    /* move list pointer to last one sent or first in list */
    listRewind(c->reply, &li);
    ln = listNext(&li);

    while(c->bufpos > c->sentlen || ln != NULL) {
        if (c->bufpos > c->sentlen) {
            nwritten = c->bufpos - c->sentlen;
            result = aeWinSocketSend(fd,c->buf+c->sentlen, nwritten,0,
                                        el, c, c->buf, sendReplyBufferDone);
            if (result == SOCKET_ERROR && errno != WSA_IO_PENDING) {
                LOG_ERROR("Error writing to client: %s", wsa_strerror(errno));
                freeClient(c);
                return;
            }
            c->sentlen += nwritten;
            totwritten += nwritten;

        } else {
            o = listNodeValue(ln);
            objlen = (int)sdslen(o->ptr);
            objmem = zmalloc_size_sds(o->ptr);

            if (objlen == 0) {
                listDelNode(c->reply,ln);
                ln = listNext(&li);
                continue;
            }

            /* object ref placed in request, release in sendReplyListDone */
            incrRefCount(o);
            result = aeWinSocketSend(fd, ((char*)o->ptr), objlen, 0,
                                        el, c, o, sendReplyListDone);
            if (result == SOCKET_ERROR && errno != WSA_IO_PENDING) {
                LOG_TRACE("Error writing to client: %s", wsa_strerror(errno));
                decrRefCount(o);
                freeClient(c);
                return;
            }
            totwritten += objlen;
            /* remove from list - object kept alive due to incrRefCount */
            listDelNode(c->reply,listFirst(c->reply));
            c->reply_bytes -= (unsigned long)objmem;
            ln = listNext(&li);
        }
        /* Note that we avoid to send more than REDIS_MAX_WRITE_PER_EVENT
         * bytes, in a single threaded server it's a good idea to serve
         * other clients as well, even if a very large request comes from
         * super fast link that is always able to accept data (in real world
         * scenario think about 'KEYS *' against the loopback interface).
         *
         * However if we are over the maxmemory limit we ignore that and
         * just deliver as much data as it is possible to deliver. */
        if (totwritten > UG_MAX_WRITE_PER_EVENT &&
            (server.maxmemory == 0 ||
             zmalloc_used_memory() < server.maxmemory)) break;
    }
    if (totwritten > 0) c->lastinteraction = server.unixtime;

}

#else
void sendReplyToClient(aeEventLoop *el, int fd, void *privdata, int mask) {
    ugClient *c = privdata;
    int nwritten = 0, totwritten = 0, objlen;
    size_t objmem;
    robj *o;
    UG_NOTUSED(el);
    UG_NOTUSED(mask);

    while(c->bufpos > 0 || listLength(c->reply)) {
        if (c->bufpos > 0) {
            if (c->flags & UG_MASTER) {
                /* Don't reply to a master */
                nwritten = c->bufpos - c->sentlen;
            } else {
                nwritten = write(fd,c->buf+c->sentlen,c->bufpos-c->sentlen);
                if (nwritten <= 0) break;
            }
            c->sentlen += nwritten;
            totwritten += nwritten;

            /* If the buffer was sent, set bufpos to zero to continue with
             * the remainder of the reply. */
            if (c->sentlen == c->bufpos) {
                c->bufpos = 0;
                c->sentlen = 0;
            }
        } else {
            o = listNodeValue(listFirst(c->reply));
            objlen = sdslen(o->ptr);
            objmem = zmalloc_size_sds(o->ptr);

            if (objlen == 0) {
                listDelNode(c->reply,listFirst(c->reply));
                continue;
            }

            if (c->flags & UG_MASTER) {
                /* Don't reply to a master */
                nwritten = objlen - c->sentlen;
            } else {
                nwritten = write(fd, ((char*)o->ptr)+c->sentlen,objlen-c->sentlen);
                if (nwritten <= 0) break;
            }
            c->sentlen += nwritten;
            totwritten += nwritten;

            /* If we fully sent the object on head go to the next one */
            if (c->sentlen == objlen) {
                listDelNode(c->reply,listFirst(c->reply));
                c->sentlen = 0;
                c->reply_bytes -= objmem;
            }
        }
        /* Note that we avoid to send more than UG_MAX_WRITE_PER_EVENT
         * bytes, in a single threaded server it's a good idea to serve
         * other clients as well, even if a very large request comes from
         * super fast link that is always able to accept data (in real world
         * scenario think about 'KEYS *' against the loopback interface).
         *
         * However if we are over the maxmemory limit we ignore that and
         * just deliver as much data as it is possible to deliver. */
        if (totwritten > UG_MAX_WRITE_PER_EVENT &&
            (server.maxmemory == 0 ||
             zmalloc_used_memory() < server.maxmemory)) break;
    }
    if (nwritten == -1) {
        if (errno == EAGAIN) {
            nwritten = 0;
        } else {
            LOG_ERROR("Error writing to client: %s", strerror(errno));
            freeClient(c);
            return;
        }
    }
    if (totwritten > 0) c->lastinteraction = server.unixtime;
    if (c->bufpos == 0 && listLength(c->reply) == 0) {
        c->sentlen = 0;
        aeDeleteFileEvent(server.el,c->fd,AE_WRITABLE);

        /* Close connection after entire reply has been sent. */
        if (c->flags & UG_CLOSE_AFTER_REPLY) freeClient(c);
    }
}
#endif


int  prepareClientToWrite(ugClient* c)
{
    if (c->fd <= 0) return UGERR; /* Fake client */
    if (listLength(c->reply) == 0 &&
            aeCreateFileEvent(server.el, c->fd, AE_WRITABLE,
                              sendReplyToClient, c) == AE_ERR) return UGERR;
    return UGOK;
}
