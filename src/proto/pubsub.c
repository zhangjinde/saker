#include "client.h"
#include "saker.h"
#include "object.h"
#include "event/util.h"
#include "utils/string.h"
#include "utils/debug.h"

typedef struct pubsubPattern {
    ugClient *client;
    robj *pattern;
} pubsubPattern;

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
    pubsubPattern *s1 = (pubsubPattern *) a;
    pubsubPattern *s2 = (pubsubPattern *) b;
    return equalObjects(s1->pattern, s2->pattern);
}

static void listFreePubsubPattern(void *ptr) {
    pubsubPattern *p = (pubsubPattern *) ptr;
    decrRefCount(p->pattern);
    zfree(p);
}

dict *createPubsubChannels() {
    return dictCreate(&callbackDict, NULL);
}

void destroyPubsubChannels(dict *channels) {
    dictRelease(channels);
}

void destroyPubsubPatterns(list *patterns) {
    listRelease(patterns);
}

list *createPubsubPatterns() {
    list *pubsubPatterns =  listCreate();
    listSetFreeMethod(pubsubPatterns, listFreePubsubPattern);
    listSetMatchMethod(pubsubPatterns, listMatchPubsubPattern);
    return pubsubPatterns;
}

void freeAllPubsubObjs() {
    dictRelease(server.pubsub_channels);
    listRelease(server.pubsub_patterns);
}

/* Subscribe a client to a channel. Returns 1 if the operation succeeded, or
 * 0 if the client was already subscribed to that channel. */
int pubsubSubscribeChannel(ugClient *c, robj *channel) {
    struct dictEntry *de;
    list *clients = NULL;
    int retval = 0;

    /* Add the channel to the client -> channels hash table */
    if (dictAdd(c->pubsub_channels, channel, NULL) == DICT_OK) {
        retval = 1;
        incrRefCount(channel);
        /* Add the client to the channel -> list of clients hash table */
        de = dictFind(server.pubsub_channels, channel);
        if (de == NULL) {
            clients = listCreate();
            dictAdd(server.pubsub_channels, channel, clients);
            incrRefCount(channel);
        } else {
            clients = dictGetEntryVal(de);
        }
        listAddNodeTail(clients,c);
    }
    /* Notify the client */
    addReply(c,shared.mbulkhdr[3]);
    addReply(c,shared.subscribebulk);
    addReplyBulk(c,channel);
    addReplyLongLong(c,dictSize(c->pubsub_channels)+listLength(c->pubsub_patterns));
    return retval;
}


int pubsubUnsubscribeChannel(ugClient *c, robj *channel, int notify) {
    struct dictEntry *de;
    list *clients;
    listNode *ln;
    int retval = 0;

    /* Remove the channel from the client -> channels hash table */
    incrRefCount(channel); /* channel may be just a pointer to the same object
                            we have in the hash tables. Protect it... */
    if (dictDelete(c->pubsub_channels,channel) == DICT_OK) {
        retval = 1;
        /* Remove the client from the channel -> clients list hash table */
        de = dictFind(server.pubsub_channels, channel);
        ugAssertWithInfo(c,NULL,de != NULL);
        clients = dictGetEntryVal(de);
        ln = listSearchKey(clients,c);
        ugAssertWithInfo(c,NULL,ln != NULL);
        listDelNode(clients,ln);
        if (listLength(clients) == 0) {
            /* Free the list and associated hash entry at all if this was
             * the latest client, so that it will be possible to abuse
             * Redis PUBSUB creating millions of channels. */
            dictDelete(server.pubsub_channels, channel);
        }
    }
    /* Notify the client */
    if (notify) {
        addReply(c,shared.mbulkhdr[3]);
        addReply(c,shared.unsubscribebulk);
        addReplyBulk(c,channel);
        addReplyLongLong(c,dictSize(c->pubsub_channels)+
                         listLength(c->pubsub_patterns));

    }
    decrRefCount(channel); /* it is finally safe to release it */
    return retval;
}

int pubsubSubscribePattern(ugClient *c, robj *pattern) {
    int retval = 0;

    if (listSearchKey(c->pubsub_patterns,pattern) == NULL) {
        pubsubPattern *pat;
        retval = 1;
        listAddNodeTail(c->pubsub_patterns,pattern);
        incrRefCount(pattern);
        pat = zmalloc(sizeof(*pat));
        pat->pattern = getDecodedObject(pattern);
        pat->client = c;
        listAddNodeTail(server.pubsub_patterns, pat);
    }
    /* Notify the client */
    addReply(c,shared.mbulkhdr[3]);
    addReply(c,shared.psubscribebulk);
    addReplyBulk(c,pattern);
    addReplyLongLong(c,dictSize(c->pubsub_channels)+listLength(c->pubsub_patterns));
    return retval;
}

/* Unsubscribe a client from a channel. Returns 1 if the operation succeeded, or
 * 0 if the client was not subscribed to the specified channel. */
int pubsubUnsubscribePattern(ugClient *c, robj *pattern, int notify) {
    listNode *ln;
    pubsubPattern pat;
    int retval = 0;

    incrRefCount(pattern); /* Protect the object. May be the same we remove */
    if ((ln = listSearchKey(c->pubsub_patterns,pattern)) != NULL) {
        retval = 1;
        listDelNode(c->pubsub_patterns,ln);
        pat.client = c;
        pat.pattern = pattern;
        ln = listSearchKey(server.pubsub_patterns, &pat);
        listDelNode(server.pubsub_patterns, ln);
    }
    /* Notify the client */
    if (notify) {
        addReply(c,shared.mbulkhdr[3]);
        addReply(c,shared.punsubscribebulk);
        addReplyBulk(c,pattern);
        addReplyLongLong(c,dictSize(c->pubsub_channels)+
                         listLength(c->pubsub_patterns));
    }
    decrRefCount(pattern);
    return retval;
}

/* Unsubscribe from all the channels. Return the number of channels the
 * client was subscribed from. */
int pubsubUnsubscribeAllChannels(ugClient *c, int notify) {
    dictIterator *di = dictGetIterator(c->pubsub_channels);
    dictEntry *de;
    int count = 0;

    while((de = dictNext(di)) != NULL) {
        robj *channel = dictGetEntryKey(de);

        count += pubsubUnsubscribeChannel(c,channel, notify);
    }
    /* We were subscribed to nothing? Still reply to the client. */
    if (notify && count == 0) {
        addReply(c,shared.mbulkhdr[3]);
        addReply(c,shared.unsubscribebulk);
        addReply(c,shared.nullbulk);
        addReplyLongLong(c,dictSize(c->pubsub_channels)+
                         listLength(c->pubsub_patterns));
    }
    dictReleaseIterator(di);
    return count;
}

/* Unsubscribe from all the patterns. Return the number of patterns the
 * client was subscribed from. */
int pubsubUnsubscribeAllPatterns(ugClient *c, int notify) {
    listNode *ln;
    listIter li;
    int count = 0;

    listRewind(c->pubsub_patterns,&li);
    while ((ln = listNext(&li)) != NULL) {
        robj *pattern = ln->value;

        count += pubsubUnsubscribePattern(c,pattern,notify);
    }
    if (notify && count == 0) {
        /* We were subscribed to nothing? Still reply to the client. */
        addReply(c,shared.mbulkhdr[3]);
        addReply(c,shared.punsubscribebulk);
        addReply(c,shared.nullbulk);
        addReplyLongLong(c,dictSize(c->pubsub_channels)+
                         listLength(c->pubsub_patterns));
    }
    return count;
}

/* Publish a message */
int pubsubPublishMessage(robj *channel, robj *message) {
    int receivers = 0;
    struct dictEntry *de;
    listNode *ln;
    listIter li;

    /* Send to clients listening for that channel */
    de = dictFind(server.pubsub_channels, channel);
    if (de) {
        list *list = dictGetEntryVal(de);
        listNode *ln;
        listIter li;

        listRewind(list,&li);
        while ((ln = listNext(&li)) != NULL) {
            ugClient *c = ln->value;

            addReply(c,shared.mbulkhdr[3]);
            addReply(c,shared.messagebulk);
            addReplyBulk(c,channel);
            addReplyBulk(c,message);
            receivers++;
        }
    }
    /* Send to clients listening to matching channels */
    if (listLength(server.pubsub_patterns)) {
        listRewind(server.pubsub_patterns,&li);
        channel = getDecodedObject(channel);
        while ((ln = listNext(&li)) != NULL) {
            pubsubPattern *pat = ln->value;

            if (stringmatchlen((char *)pat->pattern->ptr,
                               (int)sdslen(pat->pattern->ptr),
                               (char *)channel->ptr,
                               (int)sdslen(channel->ptr),0)) {
                addReply(pat->client,shared.mbulkhdr[4]);
                addReply(pat->client,shared.pmessagebulk);
                addReplyBulk(pat->client,pat->pattern);
                addReplyBulk(pat->client,channel);
                addReplyBulk(pat->client,message);
                receivers++;
            }
        }
        decrRefCount(channel);
    }
    return receivers;
}



/*-----------------------------------------------------------------------------
 * Pubsub commands implementation
 *----------------------------------------------------------------------------*/

void subscribeCommand(ugClient *c) {
    int j;

    for (j = 1; j < c->argc; j++)
        pubsubSubscribeChannel(c,c->argv[j]);
}

void unsubscribeCommand(ugClient *c) {
    if (c->argc == 1) {
        pubsubUnsubscribeAllChannels(c, 1);
    } else {
        int j;

        for (j = 1; j < c->argc; j++)
            pubsubUnsubscribeChannel(c,c->argv[j], 1);
    }
}

void psubscribeCommand(ugClient *c) {
    int j;

    for (j = 1; j < c->argc; j++)
        pubsubSubscribePattern(c, c->argv[j]);
}

void punsubscribeCommand(ugClient *c) {
    if (c->argc == 1) {
        pubsubUnsubscribeAllPatterns(c, 1);
    } else {
        int j;

        for (j = 1; j < c->argc; j++)
            pubsubUnsubscribePattern(c,c->argv[j], 1);
    }
}

void publishCommand(ugClient *c) {
    int receivers;
    if (c->argc != 3) {
        addReplyErrorFormat(c, "wrong number of arguments for '%s'", c->argv[0]);
        return ;
    }
    receivers = pubsubPublishMessage(c->argv[1],c->argv[2]);
    addReplyLongLong(c,receivers);
}
