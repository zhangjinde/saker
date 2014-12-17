#include "top.h"
#include "saker.h"
#include <string.h>

uint32_t g_time = 0;

void freeProcess(struct ProcessInfo *p) {
    if (p->name)  zfree(p->name);
    if (p->fullname) zfree(p->fullname);
    zfree(p);
}

static unsigned int callbackHash(const void *key) {
    return *(pid_t *) key;
}

static void *callbackKeyDup(void *privdata, const void *key) {
    pid_t *mykey = (pid_t *) zmalloc(sizeof(pid_t));
    UG_NOTUSED(privdata);
    memcpy(mykey, key, sizeof(pid_t));
    return mykey;
}

static int callbackKeyCompare(void *privdata, const void *key1, const void *key2) {
    UG_NOTUSED(privdata);
    return memcmp(key1, key2, sizeof(pid_t)) == 0;
}

static void callbackKeyDestructor(void *privdata, void *key) {
    UG_NOTUSED(privdata);
    zfree((pid_t *)key);
}

static void callbackValDestructor(void *privdata, void *val) {
    ((void) privdata);
    freeProcess((struct ProcessInfo *) val);
}

static dictType callbackDict = {
    callbackHash,
    callbackKeyDup,
    NULL,
    callbackKeyCompare,
    callbackKeyDestructor,
    callbackValDestructor
};

void deleteProcess(pid_t pid) {
    dictDelete(server.process, &pid);
}

struct ProcessInfo *newProcess(pid_t pid) {
    struct ProcessInfo *p = zmalloc(sizeof(struct ProcessInfo));
    memset(p, 0 ,sizeof(struct ProcessInfo));
    p->pid = pid;
    dictAdd(server.process, &pid, p);
    return p;
}

struct ProcessInfo *findProcess(pid_t pid) {
    dictEntry *de = dictFind(server.process, &pid);
    if (de) return dictGetEntryVal(de);
    return NULL;
}

void processCleanup(void) {
    dictIterator *di = dictGetIterator(server.process);
    dictEntry *de;
    while ((de = dictNext(di)) != NULL) {
        struct ProcessInfo *p = dictGetEntryVal(de);
        if (p->time_stamp != g_time) deleteProcess(p->pid);
    }
    dictReleaseIterator(di);
}

struct ProcessInfo *getProcessInfoByID(pid_t pid) {
    struct ProcessInfo *p = findProcess(pid);
    if (!p) {
        topUpdate();
        p = findProcess(pid);
    }
    return p;
}

int   topIsRuning(void) {
    return server.config->top_mode;
}

dict *createTop(void) {
    return dictCreate(&callbackDict, NULL);
}

void  destroyTop(dict *p) {
    dictRelease(p);
}
