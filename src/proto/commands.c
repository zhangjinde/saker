#include "commands.h"
#include "client.h"
#include "object.h"
#include "pubsub.h"
#include "luaworking.h"
#include "utils/xstring.h"
#include "utils/file.h"
#include "utils/debug.h"
#include "service/register.h"
#include "saker.h"

static void authCommand(ugClient *c);

static void usageCommand(ugClient *c);

static void pingCommand(ugClient *c);

static void infoCommand(ugClient *c);

static void quitCommand(ugClient *c);

static void shutdownCommand(ugClient *c);

static void loadCommand(ugClient *c);

static void unloadCommand(ugClient *c);

static void execCommand(ugClient *c);

static  ugCommand redisCommandTable[] = {
    {"auth", authCommand, 1, 0, 0},
    {"usage", usageCommand , 0, 0, 0},
    {"ping", pingCommand ,  0, 0, 0},
    {"info", infoCommand ,  0, 0, 0},
    {"quit", quitCommand ,  0, 0, 0},
    {"shutdown", shutdownCommand ,  0, 0, 0},
    {"help", usageCommand , 0, 0, 0},

    {"subscribe", subscribeCommand ,  -1, 0, 0},
    {"unsubscribe", unsubscribeCommand , -1, 0, 0},
    {"psubscribe", psubscribeCommand , 2, 0, 0},
    {"publish", publishCommand ,  2, 0, 0},

    {"load", loadCommand , -1, 0, 0},
    {"unload", unloadCommand , -1, 0, 0},
    {"exec", execCommand , -1, 0, 0},

    {NULL, NULL, 0, 0, 0}
};


static void usageCommand(ugClient *c) {
    sds usage = sdsnew(" usage:\n"
                       "help[?][usage]   Print this usage infomation.\n"
                       "ping             Ping the server.\n"
                       "info             Get information and statistics about the server.\n"
                       "shutdown         Shutdown server.\n"
                       "load             Load lua file and register lua task.\n"
                       "unload           Unregister lua task.\n"
                       "subscribe        Listen for messages published to the given channels.\n"
                       "unsubscribe      Stop listening for messages posted to the given channels.\n"
                       "psubscribe       Listen for messages published to channels matching the given patterns.\n"
                       "punsubscribe     Stop listening for messages posted to channels matching the given patterns.\n"
                       "publish          Post a message to a channel.\n"
                      );
    addReplySds(c,sdscatprintf(sdsempty(),"$%lu\r\n",
                               (unsigned long)sdslen(usage)));
    addReplySds(c, usage);
    addReply(c,shared.crlf);
}

static void authCommand(ugClient *c) {
    if (server.config->password == NULL) {
        addReplyError(c,"Client sent AUTH, but no password is set");
    } else if (strcmp(c->argv[1]->ptr, server.config->password) == 0) {
        c->authenticated = 1;
        addReplyStatus(c, "ok");
    } else {
        c->authenticated = 0;
        addReplyError(c,"invalid password.");
    }
}

static void pingCommand(ugClient *c) {
    addReply(c, shared.pong);
}

static void quitCommand(ugClient *c) {
    c->flags |= UG_CLOSE_AFTER_REPLY;
    addReply(c, shared.ok);
}


static void shutdownCommand(ugClient *c) {
    freeServer(&server);
    exit(0);
}

static void infoCommand(ugClient *c) {
    dict *ht = server.tasks;
    dictIterator *di = dictGetIterator(ht);
    dictEntry *de;
    sds s = sdsempty();

    while ((de = dictNext(di)) != NULL) {
        ugTaskType *task = dictGetEntryVal(de);
        s = sdscatprintf(s, "%-50s  %-10s \r\n", task->alias, property_string[task->property]);
    }
    dictReleaseIterator(di);
    addReplySds(c,sdscatprintf(sdsempty(),"$%lu\r\n",
                               (unsigned long)sdslen(s)));
    addReplySds(c, s);
    addReply(c,shared.crlf);
}

static void loadCommand(ugClient *c) {
    char err[LUAWORK_ERR_LEN] = {0};
    char path[MAX_STRING_LEN] = {0};
    char suffix = 0;
    dictEntry *node;
    int idx ;
    if (c->argc == 1) {
        addReplyErrorFormat(c,  "wrong number of arguments for '%s' command",  c->argv[0]) ;
        return;
    }
    suffix = server.config->script_dir[strlen(server.config->script_dir)-1];
    for (idx = 1; idx < c->argc; ++idx) {
        sds param =  c->argv[idx]->ptr;
        if ( suffix == '/' || suffix == '\\') sprintf(path, "%s%s", server.config->script_dir, param);
        else sprintf(path, "%s/%s", server.config->script_dir, param);
        if (xfileisregular(path) == UGOK) {
            if (UGOK != luaworkDoFile(server.ls, path, err)) {
                addReplyErrorFormat(c,  "load file failed '%s' '%s'",  param, err) ;
            } else {
                addReplyStatusFormat(c, "load file successed '%s'",  param);
            }
        } else if ((node=dictFind(server.tasks, param)) != NULL) {
            ugTaskType *ptask = dictGetEntryVal(node);
            if (ptask->property == PROP_UNACTIVE) {
                ptask->property = ptask->oldproperty;
                addReplyStatusFormat(c, "load alias successed '%s'", param);
            } else {
                addReplyStatusFormat(c, "don't need reload '%s'", param);
            }
        } else {
            addReplyErrorFormat(c,  "can not found '%s'", param) ;
        }
    }
}


static void execCommand(ugClient *c) {
    dictEntry *node;
    /* param index */
    int   idx=1;
    robj *primarykey = c->argv[0];
    if (strcasecmp(c->argv[0]->ptr, "exec")==0) {
        if (c->argc == 1) {
            addReplyErrorFormat(c,  "wrong number of arguments for '%s' command",  c->argv[0]) ;
            return;
        }
        primarykey = c->argv[1];
        idx = 2;
    }

    if ((node=dictFind(server.tasks, primarykey->ptr)) != NULL) {
        ugTaskType *ptask = dictGetEntryVal(node);
        const char *pret = NULL;
        int    ret = 0;
        int   top = lua_gettop(server.ls);
        int luatbl_idx = 1;
        /* the same as :
                   lua_getglobal(server.ls, ptask->func);
              */
        lua_rawgeti(server.ls, LUA_REGISTRYINDEX, ptask->handle);
        lua_createtable(server.ls, c->argc - idx, 0);
        for (; idx < c->argc; idx++) {
            /* lua index must start 1 */
            lua_pushinteger(server.ls, luatbl_idx++);
            lua_pushstring(server.ls, c->argv[idx]->ptr);
            lua_settable(server.ls , -3);
            /* the same as :
                    lua_pushstring(server.ls, c->argv[idx]);
                    lua_rawseti(server.ls, -2, idx-1);
                    */
        }

        if ((ret = lua_pcall(server.ls, 1, 2, 0)) != 0) {
            addReplyErrorFormat(c, "exec failed for '%s' errmsg:'%s'", ptask->func, lua_tostring(server.ls, -1));
            lua_pop(server.ls, 1);
            return ;
        }

        ugAssert(lua_isboolean(server.ls, -2));
        ret = lua_tointeger(server.ls, -2);

        if (!lua_isnil(server.ls, -1))  pret = lua_tostring(server.ls, -1);

        if (pret) {
            robj *o = createStringObject((char *)pret, strlen(pret));
            addReplyBulk(c, o);
            decrRefCount(o);
        } else {
            if (ret == UGOK) addReplyStatusFormat(c, "exec successed for '%s'", ptask->func);
            else addReplyErrorFormat(c, "exec successed ,but return fail for '%s'", ptask->func );
        }

        /* consume stack ,avoid memory increase*/
        lua_settop(server.ls, top);
    } else {
        addReplyErrorFormat(c, "can not find for 'exec' '%s'", primarykey->ptr);
    }
}


static void unloadCommand(ugClient *c) {
    dictEntry *node;
    int idx ;
    if (c->argc == 1) {
        addReplyErrorFormat(c,  "wrong number of arguments for '%s' command",  c->argv[0]->ptr) ;
        return;
    }
    for (idx = 1; idx<c->argc; ++idx) {
        sds param = c->argv[idx]->ptr;
        if ((node=dictFind(server.tasks, param)) != NULL) {
            ugTaskType *ptask = dictGetEntryVal(node);
            ptask->property = PROP_UNACTIVE;
            addReplyStatusFormat(c, "unload alias successed '%s'", param) ;
        } else {
            addReplyErrorFormat(c, "can not found '%s'", param) ;
        }
    }
}



/* Functions managing dictionary of callbacks for pub/sub. */
static unsigned int callbackHash(const void *key) {
    return dictGenHashFunction((unsigned char *)key,strlen((const char *)key));
}

static int callbackKeyCompare(void *privdata, const void *key1, const void *key2) {
    UG_NOTUSED(privdata);
    return strcasecmp(key1, key2)==0;
}

static void callbackKeyDestructor(void *privdata, void *key) {
    UG_NOTUSED(privdata);
    zfree((char *)key);
}

static dictType callbackDict = {
    callbackHash,
    NULL,
    NULL,
    callbackKeyCompare,
    callbackKeyDestructor,
    NULL
};

dict *createCommands() {
    int idx = 0;
    dict *commands = dictCreate(&callbackDict, NULL);

    for( ; redisCommandTable[idx].name; ++idx) {
        int retval;
        retval = dictAdd(commands, xstrdup(redisCommandTable[idx].name), &(redisCommandTable[idx]) );
        ugAssert(retval == DICT_OK);
    }
    return commands;
}

void destroyCommands(dict *commands) {
    dictRelease(commands);
}

ugCommand *lookupCommand(char *key) {
    dictEntry *de = dictFind(server.commands, key);
    if (de) {
        return dictGetEntryVal(de);
    }
    /* custom cmd */
    else if (dictFind(server.tasks, key)) {
        de = dictFind(server.commands, "exec");
        if (de) return dictGetEntryVal(de);
    }
    return NULL;
}
