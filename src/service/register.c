#include "register.h"
#include <string.h>

#include "saker.h"
#include "utils/logger.h"
#include "utils/string.h"
#include "utils/debug.h"
#include "proto/client.h"
#include "proto/pubsub.h"
#include "proto/object.h"

const char* property_string[] = {
    "unknow",
    "unactive",
    "once",
    "cycle",
    "passivity",
    "defer"
};

ugTaskType* createTaskObj() 
{
    ugTaskType* obj = zmalloc(sizeof(ugTaskType));
    memset(obj, 0 ,sizeof(ugTaskType));
    obj->prior =  PRIO_LOWEST;
    return obj;
}

void freeTaskObj(void* obj) 
{
    ugTaskType* p = (ugTaskType*) obj;
    zfree(p->alias);
    zfree(p->func);
    zfree(p) ;
}

/* Functions managing dictionary of callbacks for pub/sub. */
static unsigned int callbackHash(const void *key) 
{
    return dictGenHashFunction((unsigned char*)key,strlen((const char*)key));
}

static void *callbackKeyDup(void *privdata, const void *key)
{
    const char * pkey = key;
    return xstrdup(pkey);
}

static void *callbackValDup(void *privdata, const void *src) 
{
    ugTaskType* dup = NULL;
    ugTaskType* srctask = (ugTaskType*) src;
    UG_NOTUSED(privdata);
    dup = (ugTaskType*)zmalloc(sizeof(ugTaskType));
    memcpy(dup,src,sizeof(ugTaskType));
    dup->alias = xstrdup(srctask->alias);
    dup->func  = xstrdup(srctask->func);
    return dup;
}

static int callbackKeyCompare(void *privdata, const void *key1, const void *key2) 
{
    int l1, l2;
    UG_NOTUSED(privdata);

    l1 = strlen((const char*)key1);
    l2 = strlen((const char*)key2);
    if (l1 != l2) return 0;
    return memcmp(key1,key2,l1) == 0;
}

static void callbackKeyDestructor(void *privdata, void *key) 
{
    UG_NOTUSED(privdata);
    zfree((char*)key);
}

static void callbackValDestructor(void *privdata, void *val) 
{
    ((void) privdata);
    freeTaskObj(val);
}

static dictType callbackDict = {
    callbackHash,
    callbackKeyDup,
    callbackValDup,
    callbackKeyCompare,
    callbackKeyDestructor,
    callbackValDestructor
};


dict*  createTaskMap() 
{
    return dictCreate(&callbackDict,NULL);
}


void  destroyTaskMap(dict* p) 
{
    if (p) {
        dictIterator* di = dictGetIterator(p);
        dictEntry* de;
        while ((de = dictNext(di)) != NULL) {
            ugTaskType* ptask = dictGetEntryVal(de); 
            luaworkUnrefFunction(server.ls, ptask->handle, NULL);            
        }
        dictReleaseIterator(di);
        dictRelease(p);
    }
}

int ugRegister(lua_State* L) 
{
    int top = lua_gettop(L);
    const char* alias ;
    const char* funcname ;
    ugTaskType*  ptask = NULL;
    dictEntry* de;
    task_property_t taskproperty = PROP_UNKNOW;
    if (top != 3) {
        lua_pushnil(L);
        lua_pushstring(L, "register requires three arguments.");
        return 2;
    }
    alias =    luaL_checkstring(L, 1);
    funcname = luaL_checkstring(L, 2);
    taskproperty = luaL_checkinteger(L, 3);
    if (!alias || !funcname) {
        lua_pushnil(L);
        lua_pushfstring(L, "register [%s]  [%s] failed", alias, funcname);
        return 2;
    }
    
    ptask = createTaskObj();
    ptask->alias = xstrdup(alias);
    ptask->func  = xstrdup(funcname);
    /*  property */
    ptask->property = ptask->oldproperty = taskproperty;   
    de = dictFind(server.tasks, ptask->alias);
    if (de) {
        ugTaskType* poldtask = dictGetEntryVal(de); 
        luaworkUnrefFunction(server.ls,  poldtask->handle, NULL);        
        dictDelete(server.tasks, ptask->alias);
    }

    ptask->handle = luaworkRefFunction(server.ls, ptask->func, NULL);

    if (dictAdd(server.tasks, ptask->alias, ptask) != DICT_OK) {
        lua_pushnil(L);  
        lua_pushfstring(L, "add key:[%s] to dictionary failed,repeated register.", ptask->alias);
        freeTaskObj(ptask);
        return 2;
    }
    freeTaskObj(ptask);
    LOG_INFO("register [%s]  [%s] success", alias, funcname);
    lua_pushboolean(L, 1); 
    return 1;
}

int ugUnregister(lua_State* L) 
{
    dictEntry* de;
    const char* alias =  luaL_checkstring(L, 1);
    if ((de=dictFind(server.tasks, alias)) != NULL) {
        ugTaskType* ptask = dictGetEntryVal(de); 
        ptask->property = PROP_UNACTIVE;
    }
    else {
        lua_pushnil(L);  
        lua_pushfstring(L, "cannot find [%s].", alias);
        return 2;
    }
    LOG_INFO("unregister [%s] success", alias);
    lua_pushboolean(L, 1);  
    return 1;
}


int   ugPublish(lua_State* L) 
{
    int top = lua_gettop(L);
	int recvs;
    robj* channel, *message;
    const char* p1, *p2;
    
    if (top != 2) {
        lua_pushnil(L);  
        lua_pushstring(L, "wrong number of arguments for publish.");
        return 2;
    }
    p1 = luaL_checkstring(L, 1);
    p2 = luaL_checkstring(L, 2);
    channel = createStringObject((char*) p1, strlen(p1));
    message = createStringObject((char*) p2, strlen(p2));

    recvs = pubsubPublishMessage(channel, message);
    decrRefCount(channel);
    decrRefCount(message);
	
	lua_pushinteger(L, recvs);
    return 1;
}
