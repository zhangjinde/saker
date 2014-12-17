#include "client.h"
#include <time.h>
#include "common/common.h"
#include "proto/object.h"
#include "saker.h"
#include "utils/logger.h"
#include "utils/error.h"

static void acceptCommonhandler(int fd, int flags) {
    ugClient *c;
    if ((c = createClient(fd)) == NULL) {
        LOG_TRACE(
            "Error registering fd event for the new client: %s (fd=%d)",
            xerrmsg(),fd);
        close(fd); /* May be already closed, just ignore errors */
        return;
    }
    /* If maxclient directive is set and this is one client more... close the
    * connection. Note that we create the client instead to check before
    * for this condition, since now the socket is already set in non-blocking
    * mode and we can send an error for free using the Kernel I/O */
    if ((unsigned long long)listLength(server.clients) > server.config->maxclients) {
        char *err = "-ERR max number of clients reached\r\n";

        /* That's a best effort error message, don't check write errors */
        if (write(c->fd,err,strlen(err)) == -1) {
            /* Nothing to do, Just to avoid the warning... */
        }
        server.stat_rejected_conn++;
        freeClient(c);
        return;
    }
    server.stat_numconnections++;
    c->flags |= flags;
}

void readQueryFromClient(aeEventLoop *el, int fd, void *privdata, int mask) {
    ugClient *c = (ugClient *) privdata;
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
    nread = read(fd, c->querybuf+qblen, readlen);
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
    if (nread) {
        sdsIncrLen(c->querybuf, nread);
        c->lastinteraction = time(NULL);
    } else {
        return;
    }
    LOG_TRACE("read buff from client : %s", c->querybuf);
    processInputBuffer(c);
}

void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask) {
    int cport, cfd;
    char cip[128];
    char errbuff[MAX_STRING_LEN] = {0};
    UG_NOTUSED(el);
    UG_NOTUSED(mask);
    UG_NOTUSED(privdata);

    cfd = anetTcpAccept(errbuff, fd, cip, sizeof(cip), &cport);
    if (cfd == AE_ERR) {
        LOG_WARNING("Accepting client connection: %s", errbuff);
        return;
    }
    LOG_TRACE("Accepted %s:%d", cip, cport);
    acceptCommonhandler(cfd, 0);
}

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
                nwritten = write(fd, ((char *)o->ptr)+c->sentlen,objlen-c->sentlen);
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

int  prepareClientToWrite(ugClient *c) {
    if (c->fd <= 0) return UGERR; /* Fake client */
    if (listLength(c->reply) == 0 &&
            aeCreateFileEvent(server.el, c->fd, AE_WRITABLE,
                              sendReplyToClient, c) == AE_ERR) return UGERR;
    return UGOK;
}
