/* Redis Object implementation.
 *
 * Copyright (c) 2009-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "object.h"
#include "utils/error.h"
#include "common/winfixes.h"
#include <math.h>
#include <ctype.h>
#include <string.h>
#include "saker.h"

#include "event/util.h"
#include "utils/sds.h"
#include "utils/udict.h"
#include "utils/debug.h"


struct sharedObjectsStruct shared;

robj *createObject(int type, void *ptr) {
    robj *o = zmalloc(sizeof(*o));
    o->type = type;
    o->encoding = UG_ENCODING_RAW;
    o->ptr = ptr;
    o->refcount = 1;

    /* Set the LRU to the current lruclock (minutes resolution). */
    o->lru = server.lruclock;
    return o;
}

robj *createStringObject(char *ptr, size_t len) {
    return createObject(UG_STRING,sdsnewlen(ptr,len));
}

robj *createStringObjectFromLongLong(long long value) {
    robj *o;
    if (value >= 0 && value < UG_SHARED_INTEGERS) {
        incrRefCount(shared.integers[value]);
        o = shared.integers[value];
    } else {
        if (value >= LONG_MIN && value <= LONG_MAX) {
            o = createObject(UG_STRING, NULL);
            o->encoding = UG_ENCODING_INT;
            o->ptr = (void*)(value);
        } else {
            o = createObject(UG_STRING,sdsfromlonglong(value));
        }
    }
    return o;
}

/* Note: this function is defined into object.c since here it is where it
 * belongs but it is actually designed to be used just for INCRBYFLOAT */
robj *createStringObjectFromLongDouble(long double value) {
    char buf[256];
    int len;

    /* We use 17 digits precision since with 128 bit floats that precision
     * after rounding is able to represent most small decimal numbers in a way
     * that is "non surprising" for the user (that is, most small decimal
     * numbers will be represented in a way that when converted back into
     * a string are exactly the same as what the user typed.) */
#ifdef _WIN32
    /* on Windows the magic number is 15 */
    len = snprintf(buf,sizeof(buf),"%.15Lf", value);
#else
    len = snprintf(buf,sizeof(buf),"%.17Lf", value);
#endif
    /* Now remove trailing zeroes after the '.' */
    if (strchr(buf,'.') != NULL) {
        char *p = buf+len-1;
        while(*p == '0') {
            p--;
            len--;
        }
        if (*p == '.') len--;
    }
    return createStringObject(buf,len);
}

robj *dupStringObject(robj *o) {
    ugAssert(o->encoding == UG_ENCODING_RAW);
    return createStringObject(o->ptr,sdslen(o->ptr));
}

robj *createListObject(void) {
    list *l = listCreate();
    robj *o = createObject(UG_LIST,l);
    listSetFreeMethod(l,decrRefCount);
    o->encoding = UG_ENCODING_LINKEDLIST;
    return o;
}

void freeStringObject(robj *o) {
    if (o->encoding == UG_ENCODING_RAW) {
        sdsfree(o->ptr);
    }
}

void freeListObject(robj *o) {
    switch (o->encoding) {
    case UG_ENCODING_LINKEDLIST:
        listRelease((list*) o->ptr);
        break;
    case UG_ENCODING_ZIPLIST:
        zfree(o->ptr);
        break;
    default:
        ugPanic("Unknown list encoding type");
    }
}

void incrRefCount(robj *o) {
    o->refcount++;
}

void decrRefCount(void *obj) {
    robj *o = obj;

    if (o->refcount <= 0) ugPanic("decrRefCount against refcount <= 0");
    if (o->refcount == 1) {
        switch(o->type) {
        case UG_STRING: freeStringObject(o); break;
        case UG_LIST: freeListObject(o); break;
        default: ugPanic("Unknown object type"); break;
        }
        zfree(o);
    } else {
        o->refcount--;
    }
}

/* This function set the ref count to zero without freeing the object.
 * It is useful in order to pass a new object to functions incrementing
 * the ref count of the received object. Example:
 *
 *    functionThatWillIncrementRefCount(resetRefCount(CreateObject(...)));
 *
 * Otherwise you need to resort to the less elegant pattern:
 *
 *    *obj = createObject(...);
 *    functionThatWillIncrementRefCount(obj);
 *    decrRefCount(obj);
 */
robj *resetRefCount(robj *obj) {
    obj->refcount = 0;
    return obj;
}

int checkType(ugClient *c, robj *o, int type) {
    if (o->type != type) {
        addReply(c,shared.wrongtypeerr);
        return 1;
    }
    return 0;
}

int isObjectRepresentableAsLongLong(robj *o, long long *llval) {
    ugAssert(o->type == UG_STRING);
    if (o->encoding == UG_ENCODING_INT) {
        if (llval) *llval = (long long) o->ptr;
        return UGOK;
    } else {
        return string2ll(o->ptr,sdslen(o->ptr),llval) ? UGOK : UGERR;
    }
}

/* Try to encode a string object in order to save space */
robj *tryObjectEncoding(robj *o) {
    long value;
    sds s = o->ptr;

    if (o->encoding != UG_ENCODING_RAW)
        return o; /* Already encoded */

    /* It's not safe to encode shared objects: shared objects can be shared
     * everywhere in the "object space" of Redis. Encoded objects can only
     * appear as "values" (and not, for instance, as keys) */
     if (o->refcount > 1) return o;

    /* Currently we try to encode only strings */
    ugAssert(o->type == UG_STRING);

    /* Check if we can represent this string as a long integer */
    if (!string2l(s,sdslen(s),&value)) return o;

    /* Ok, this object can be encoded...
     *
     * Can I use a shared object? Only if the object is inside a given range
     *
     * Note that we also avoid using shared integers when maxmemory is used
     * because every object needs to have a private LRU field for the LRU
     * algorithm to work well. */
    if ( value >= 0 && value < UG_SHARED_INTEGERS) {
        decrRefCount(o);
        incrRefCount(shared.integers[value]);
        return shared.integers[value];
    } else {
        o->encoding = UG_ENCODING_INT;
        sdsfree(o->ptr);
        o->ptr = (void*) value;
        return o;
    }
}

/* Get a decoded version of an encoded object (returned as a new object).
 * If the object is already raw-encoded just increment the ref count. */
robj *getDecodedObject(robj *o) {
    robj *dec;

    if (o->encoding == UG_ENCODING_RAW) {
        incrRefCount(o);
        return o;
    }
    if (o->type == UG_STRING && o->encoding == UG_ENCODING_INT) {
        char buf[32];

        ll2string(buf,32,(long)o->ptr);
        dec = createStringObject(buf,strlen(buf));
        return dec;
    } else {
        ugPanic("Unknown encoding type");
    }
}

/* Compare two string objects via strcmp() or alike.
 * Note that the objects may be integer-encoded. In such a case we
 * use ll2string() to get a string representation of the numbers on the stack
 * and compare the strings, it's much faster than calling getDecodedObject().
 *
 * Important note: if objects are not integer encoded, but binary-safe strings,
 * sdscmp() from sds.c will apply memcmp() so this function ca be considered
 * binary safe. */
int compareStringObjects(robj *a, robj *b) {
    char bufa[128], bufb[128], *astr, *bstr;
    int bothsds = 1;
    ugAssert(a->type == UG_STRING && b->type == UG_STRING);

    if (a == b) return 0;
    if (a->encoding != UG_ENCODING_RAW) {
        ll2string(bufa,sizeof(bufa),(long) a->ptr);
        astr = bufa;
        bothsds = 0;
    } else {
        astr = a->ptr;
    }
    if (b->encoding != UG_ENCODING_RAW) {
        ll2string(bufb,sizeof(bufb),(long) b->ptr);
        bstr = bufb;
        bothsds = 0;
    } else {
        bstr = b->ptr;
    }
    return bothsds ? sdscmp(astr,bstr) : strcmp(astr,bstr);
}

/* Equal string objects return 1 if the two objects are the same from the
 * point of view of a string comparison, otherwise 0 is returned. Note that
 * this function is faster then checking for (compareStringObject(a,b) == 0)
 * because it can perform some more optimization. */
int equalStringObjects(robj *a, robj *b) {
    if (a->encoding != UG_ENCODING_RAW && b->encoding != UG_ENCODING_RAW){
        return a->ptr == b->ptr;
    } else {
        return compareStringObjects(a,b) == 0;
    }
}

size_t stringObjectLen(robj *o) {
    ugAssert(o->type == UG_STRING);
    if (o->encoding == UG_ENCODING_RAW) {
        return sdslen(o->ptr);
    } else {
        char buf[32];

        return ll2string(buf,32,(long)o->ptr);
    }
}

int getDoubleFromObject(robj *o, double *target) {
    double value;
    char *eptr;

    if (o == NULL) {
        value = 0;
    } else {
        ugAssert(o->type == UG_STRING);
        if (o->encoding == UG_ENCODING_RAW) {
            errno = 0;
            value = strtod(o->ptr, &eptr);
            if (isspace(((char*)o->ptr)[0]) || eptr[0] != '\0' ||
                errno == ERANGE || isnan(value))
                return UGERR;
        } else if (o->encoding == UG_ENCODING_INT) {
            value = (long)o->ptr;
        } else {
            ugPanic("Unknown string encoding");
        }
    }
    *target = value;
    return UGOK;
}

int getDoubleFromObjectOrReply(ugClient *c, robj *o, double *target, const char *msg) {
    double value;
    if (getDoubleFromObject(o, &value) != UGOK) {
        if (msg != NULL) {
            addReplyErrorFormat(c,(char*)msg);
        } else {
            addReplyErrorFormat(c,"value is not a valid float");
        }
        return UGERR;
    }
    *target = value;
    return UGOK;
}

int getLongDoubleFromObject(robj *o, long double *target) {
    long double value;
    char *eptr;

    if (o == NULL) {
        value = 0;
    } else {
        ugAssert(o->type == UG_STRING);
        if (o->encoding == UG_ENCODING_RAW) {
            errno = 0;
#ifdef _WIN32
            value = wstrtod(o->ptr, &eptr);
#else
            value = strtold(o->ptr, &eptr);
#endif
            if (isspace(((char*)o->ptr)[0]) || eptr[0] != '\0' ||
                errno == ERANGE || isnan(value))
                return UGERR;
        } else if (o->encoding == UG_ENCODING_INT) {
            value = (long)o->ptr;
        } else {
            ugPanic("Unknown string encoding");
        }
    }
    *target = value;
    return UGOK;
}

int getLongDoubleFromObjectOrReply(ugClient *c, robj *o, long double *target, const char *msg) {
    long double value;
    if (getLongDoubleFromObject(o, &value) != UGOK) {
        if (msg != NULL) {
            addReplyErrorFormat(c,(char*)msg);
        } else {
            addReplyErrorFormat(c,"value is not a valid float");
        }
        return UGERR;
    }
    *target = value;
    return UGOK;
}

int getLongLongFromObject(robj *o, long long *target) {
    long long value;
    char *eptr;

    if (o == NULL) {
        value = 0;
    } else {
        ugAssert(o->type == UG_STRING);
        if (o->encoding == UG_ENCODING_RAW) {
            errno = 0;
            value = strtoll(o->ptr, &eptr, 10);
            if (isspace(((char*)o->ptr)[0]) || eptr[0] != '\0' ||
                errno == ERANGE)
                return UGERR;
        } else if (o->encoding == UG_ENCODING_INT) {
            value = (long)o->ptr;
        } else {
            ugPanic("Unknown string encoding");
        }
    }
    if (target) *target = value;
    return UGOK;
}

int getLongLongFromObjectOrReply(ugClient *c, robj *o, long long *target, const char *msg) {
    long long value;
    if (getLongLongFromObject(o, &value) != UGOK) {
        if (msg != NULL) {
            addReplyErrorFormat(c,(char*)msg);
        } else {
            addReplyErrorFormat(c,"value is not an integer or out of range");
        }
        return UGERR;
    }
    *target = value;
    return UGOK;
}

int getLongFromObjectOrReply(ugClient *c, robj *o, long *target, const char *msg) {
    long long value;

    if (getLongLongFromObjectOrReply(c, o, &value, msg) != UGOK) return UGERR;
    if (value < LONG_MIN || value > LONG_MAX) {
        if (msg != NULL) {
            addReplyErrorFormat(c,(char*)msg);
        } else {
            addReplyErrorFormat(c,"value is out of range");
        }
        return UGERR;
    }
    *target = (long)value;
    return UGOK;
}

char *strEncoding(int encoding) {
    switch(encoding) {
    case UG_ENCODING_RAW: return "raw";
    case UG_ENCODING_INT: return "int";
    case UG_ENCODING_HT: return "hashtable";
    case UG_ENCODING_SKIPLIST: return "skiplist";
    default: return "unknown";
    }
}

/* Given an object returns the min number of seconds the object was never
 * requested, using an approximated LRU algorithm. */
unsigned long estimateObjectIdleTime(robj *o) {
    if (server.lruclock >= o->lru) {
        return (server.lruclock - o->lru) * UG_LRU_CLOCK_RESOLUTION;
    } else {
        return ((UG_LRU_CLOCK_MAX - o->lru) + server.lruclock) *
                    UG_LRU_CLOCK_RESOLUTION;
    }
}

/* This is an helper function for the DEBUG command. We need to lookup keys
 * without any modification of LRU or other parameters. */
robj *objectCommandLookup(ugClient *c, robj *key) {
    //dictEntry *de;

    //if ((de = dictFind(c->db->dict,key->ptr)) == NULL) return NULL;
    //return (robj*) dictGetVal(de);
    return NULL;
}

robj *objectCommandLookupOrReply(ugClient *c, robj *key, robj *reply) {
    robj *o = objectCommandLookup(c,key);

    if (!o) addReply(c, reply);
    return o;
}

void createSharedObjects(void) {
    int j;

    shared.crlf = createObject(UG_STRING,sdsnew("\r\n"));
    shared.ok = createObject(UG_STRING,sdsnew("+OK\r\n"));
    shared.err = createObject(UG_STRING,sdsnew("-ERR\r\n"));
    shared.emptybulk = createObject(UG_STRING,sdsnew("$0\r\n\r\n"));
    shared.czero = createObject(UG_STRING,sdsnew(":0\r\n"));
    shared.cone = createObject(UG_STRING,sdsnew(":1\r\n"));
    shared.cnegone = createObject(UG_STRING,sdsnew(":-1\r\n"));
    shared.nullbulk = createObject(UG_STRING,sdsnew("$-1\r\n"));
    shared.nullmultibulk = createObject(UG_STRING,sdsnew("*-1\r\n"));
    shared.emptymultibulk = createObject(UG_STRING,sdsnew("*0\r\n"));
    shared.pong = createObject(UG_STRING,sdsnew("+PONG\r\n"));
    shared.queued = createObject(UG_STRING,sdsnew("+QUEUED\r\n"));
    shared.wrongtypeerr = createObject(UG_STRING,sdsnew(
        "-ERR Operation against a key holding the wrong kind of value\r\n"));
    shared.nokeyerr = createObject(UG_STRING,sdsnew(
        "-ERR no such key\r\n"));
    shared.syntaxerr = createObject(UG_STRING,sdsnew(
        "-ERR syntax error\r\n"));
    shared.sameobjecterr = createObject(UG_STRING,sdsnew(
        "-ERR source and destination objects are the same\r\n"));
    shared.outofrangeerr = createObject(UG_STRING,sdsnew(
        "-ERR index out of range\r\n"));
    shared.noscripterr = createObject(UG_STRING,sdsnew(
        "-NOSCRIPT No matching script. Please use EVAL.\r\n"));
    shared.loadingerr = createObject(UG_STRING,sdsnew(
        "-LOADING Redis is loading the dataset in memory\r\n"));
    shared.slowscripterr = createObject(UG_STRING,sdsnew(
        "-BUSY Redis is busy running a script. You can only call SCRIPT KILL or SHUTDOWN NOSAVE.\r\n"));
    shared.masterdownerr = createObject(UG_STRING,sdsnew(
        "-MASTERDOWN Link with MASTER is down and slave-serve-stale-data is set to 'no'.\r\n"));
    shared.bgsaveerr = createObject(UG_STRING,sdsnew(
        "-MISCONF Redis is configured to save RDB snapshots, but is currently not able to persist on disk. Commands that may modify the data set are disabled. Please check Redis logs for details about the error.\r\n"));
    shared.roslaveerr = createObject(UG_STRING,sdsnew(
        "-READONLY You can't write against a read only slave.\r\n"));
    shared.oomerr = createObject(UG_STRING,sdsnew(
        "-OOM command not allowed when used memory > 'maxmemory'.\r\n"));
    shared.execaborterr = createObject(UG_STRING,sdsnew(
        "-EXECABORT Transaction discarded because of previous errors.\r\n"));
    shared.space = createObject(UG_STRING,sdsnew(" "));
    shared.colon = createObject(UG_STRING,sdsnew(":"));
    shared.plus = createObject(UG_STRING,sdsnew("+"));

    for (j = 0; j < UG_SHARED_SELECT_CMDS; j++) {
        shared.select[j] = createObject(UG_STRING,
            sdscatprintf(sdsempty(),"select %d\r\n", j));
    }
    shared.messagebulk = createStringObject("$7\r\nmessage\r\n",13);
    shared.pmessagebulk = createStringObject("$8\r\npmessage\r\n",14);
    shared.subscribebulk = createStringObject("$9\r\nsubscribe\r\n",15);
    shared.unsubscribebulk = createStringObject("$11\r\nunsubscribe\r\n",18);
    shared.psubscribebulk = createStringObject("$10\r\npsubscribe\r\n",17);
    shared.punsubscribebulk = createStringObject("$12\r\npunsubscribe\r\n",19);
    shared.del = createStringObject("DEL",3);
    shared.rpop = createStringObject("RPOP",4);
    shared.lpop = createStringObject("LPOP",4);
    shared.lpush = createStringObject("LPUSH",5);
    for (j = 0; j < UG_SHARED_INTEGERS; j++) {
        shared.integers[j] = createObject(UG_STRING,(void*)(long)j);
        shared.integers[j]->encoding = UG_ENCODING_INT;
    }
    for (j = 0; j < UG_SHARED_BULKHDR_LEN; j++) {
        shared.mbulkhdr[j] = createObject(UG_STRING,
            sdscatprintf(sdsempty(),"*%d\r\n",j));
        shared.bulkhdr[j] = createObject(UG_STRING,
            sdscatprintf(sdsempty(),"$%d\r\n",j));
    }
}

void destroySharedObjects(void) 
{
    int j;
    decrRefCount(shared.crlf);
    decrRefCount(shared.ok);
    decrRefCount(shared.err);
    decrRefCount(shared.emptybulk);
    decrRefCount(shared.czero);
    decrRefCount(shared.cone );
    decrRefCount(shared.cnegone );
    decrRefCount(shared.nullbulk);
    decrRefCount(shared.nullmultibulk);
    decrRefCount(shared.emptymultibulk );
    decrRefCount(shared.pong );
    decrRefCount(shared.queued );
    decrRefCount(shared.wrongtypeerr); 
    decrRefCount(shared.nokeyerr); 
    decrRefCount(shared.syntaxerr );
    decrRefCount(shared.sameobjecterr); 
    decrRefCount(shared.outofrangeerr );
    decrRefCount(shared.noscripterr); 
    decrRefCount(shared.loadingerr); 
    decrRefCount(shared.slowscripterr );
    decrRefCount(shared.masterdownerr); 
    decrRefCount(shared.bgsaveerr); 
    decrRefCount(shared.roslaveerr );
    decrRefCount(shared.oomerr); 
    decrRefCount(shared.execaborterr); 
    decrRefCount(shared.space );
    decrRefCount(shared.colon );
    decrRefCount(shared.plus); 

    for (j = 0; j < UG_SHARED_SELECT_CMDS; j++) {
        decrRefCount(shared.select[j] );
    }
    decrRefCount(shared.messagebulk );
    decrRefCount(shared.pmessagebulk );
    decrRefCount(shared.subscribebulk );
    decrRefCount(shared.unsubscribebulk);
    decrRefCount(shared.psubscribebulk );
    decrRefCount(shared.punsubscribebulk );
    decrRefCount(shared.del );
    decrRefCount(shared.rpop );
    decrRefCount(shared.lpop );
    decrRefCount(shared.lpush);
    for (j = 0; j < UG_SHARED_INTEGERS; j++) {
        decrRefCount(shared.integers[j] );
    }
    for (j = 0; j < UG_SHARED_BULKHDR_LEN; j++) {
        decrRefCount(shared.mbulkhdr[j]); 
        decrRefCount(shared.bulkhdr[j] );
    }

}

int equalObjects( robj* a, robj* b )
{
    int cmp;
    int l1,l2;
    if (a->encoding == UG_ENCODING_INT &&
        b->encoding == UG_ENCODING_INT)
        return a->ptr == b->ptr;

    a = getDecodedObject(a);
    b = getDecodedObject(b);
    l1 = (int)sdslen((sds)a->ptr);
    l2 = (int)sdslen((sds)b->ptr);
    if (l1 != l2) cmp = 0;
    else cmp = memcmp(a->ptr, b->ptr, l1) == 0;
    decrRefCount(a);
    decrRefCount(b);
    return cmp;
}



