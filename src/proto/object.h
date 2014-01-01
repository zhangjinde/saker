#ifndef _OBJECT__H_
#define _OBJECT__H_

#include "common/common.h"
#include "client.h"

/* Object types */
#define UG_STRING 0
#define UG_LIST 1
#define UG_HASH 4

#define UG_SHARED_SELECT_CMDS 10
#define UG_SHARED_INTEGERS 10000
#define UG_SHARED_BULKHDR_LEN 32



/* Objects encoding. Some kind of objects like Strings and Hashes can be
 * internally represented in multiple ways. The 'encoding' field of the object
 * is set to one of this fields for this object. */
#define UG_ENCODING_RAW 0     /* Raw representation */
#define UG_ENCODING_INT 1     /* Encoded as integer */
#define UG_ENCODING_HT 2      /* Encoded as hash table */
#define UG_ENCODING_ZIPMAP 3  /* Encoded as zipmap */
#define UG_ENCODING_LINKEDLIST 4 /* Encoded as regular linked list */
#define UG_ENCODING_ZIPLIST 5 /* Encoded as ziplist */
#define UG_ENCODING_INTSET 6  /* Encoded as intset */
#define UG_ENCODING_SKIPLIST 7  /* Encoded as skiplist */


#define UG_LRU_CLOCK_MAX ((1<<21)-1) /* Max value of obj->lru */
#define UG_LRU_CLOCK_RESOLUTION 10 /* LRU clock resolution in seconds */

typedef struct protoObject {
    unsigned type:4;
    unsigned notused:2;     /* Not used */
    unsigned encoding:4;
    unsigned lru:22;        /* lru time (relative to server.lruclock) */
    int refcount;
    void *ptr;
} robj;

struct sharedObjectsStruct {
    robj *crlf, *ok, *err, *emptybulk, *czero, *cone, *cnegone, *pong, *space,
    *colon, *nullbulk, *nullmultibulk, *queued,
    *emptymultibulk, *wrongtypeerr, *nokeyerr, *syntaxerr, *sameobjecterr,
    *outofrangeerr, *noscripterr, *loadingerr, *slowscripterr, *bgsaveerr,
    *masterdownerr, *roslaveerr, *execaborterr,
    *oomerr, *plus, *messagebulk, *pmessagebulk, *subscribebulk,
    *unsubscribebulk, *psubscribebulk, *punsubscribebulk, *del, *rpop, *lpop,
    *lpush,
    *select[UG_SHARED_SELECT_CMDS],
    *integers[UG_SHARED_INTEGERS],
    *mbulkhdr[UG_SHARED_BULKHDR_LEN], /* "*<value>\r\n" */
    *bulkhdr[UG_SHARED_BULKHDR_LEN];  /* "$<value>\r\n" */
};

extern struct sharedObjectsStruct shared;

void createSharedObjects(void) ;

void destroySharedObjects(void) ;

robj *createObject(int type, void *ptr);

robj *createStringObject(char *ptr, size_t len);

robj *createStringObjectFromLongLong(long long value) ;

robj *dupStringObject(robj *o);

robj *createListObject(void);

void freeStringObject(robj *o);

void freeListObject(robj *o);


void incrRefCount(robj *o) ;

void decrRefCount(void *obj);

robj *resetRefCount(robj *obj);

int checkType(ugClient *c, robj *o, int type) ;

int isObjectRepresentableAsLongLong(robj *o, long long *llval);

robj *tryObjectEncoding(robj *o);


robj *getDecodedObject(robj *o) ;

int compareStringObjects(robj *a, robj *b);

int equalObjects(robj* a, robj* b);

int equalStringObjects(robj *a, robj *b);

size_t stringObjectLen(robj *o);

int getDoubleFromObject(robj *o, double *target);

int getDoubleFromObjectOrReply(ugClient *c, robj *o, double *target, const char *msg);

int getLongDoubleFromObject(robj *o, long double *target) ;

int getLongDoubleFromObjectOrReply(ugClient *c, robj *o, long double *target, const char *msg);

int getLongLongFromObject(robj *o, long long *target) ;

int getLongLongFromObjectOrReply(ugClient *c, robj *o, long long *target, const char *msg);

int getLongFromObjectOrReply(ugClient *c, robj *o, long *target, const char *msg) ;

char *strEncoding(int encoding);

unsigned long estimateObjectIdleTime(robj *o) ;

robj *objectCommandLookup(ugClient *c, robj *key) ;

robj *objectCommandLookupOrReply(ugClient *c, robj *key, robj *reply);

robj *createStringObjectFromLongDouble(long double value) ;


#endif
