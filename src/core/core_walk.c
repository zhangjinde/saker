#include "core_declarer.h"
#include "utils/ulist.h"
#include "utils/string.h"
#include "utils/file.h"
#include "utils/sds.h"

static void freebuff(void *p) {
    char *buff = (char *)p;
    if (buff) {
        zfree(buff);
        buff = NULL;
    }
}

int core_walk(lua_State *L) {
    sds relpath = sdsempty();
    const char *path = lua_tostring(L, 1);
    list *dirqueue = listCreate();
    const char *match = NULL;
    int idx = 1;
    if (lua_gettop(L) == 2) {
        match = lua_tostring(L, 2);
    }
    listSetFreeMethod(dirqueue, freebuff);
    listAddNodeHead(dirqueue, xstrdup(path));
    lua_newtable(L);
    while (listLength(dirqueue)) {
        listNode *head = listFirst(dirqueue);
        list *queue = listCreate();
        listNode *node=NULL;
        listIter *iter = NULL;
        if (xfilelistdir((char *)head->value, "*", queue) == UGERR) {
            sdsfree(relpath);
            listRelease(dirqueue);
            listRelease(queue);
            lua_pop(L, 1); /* pop table */
            lua_pushnil(L);
            lua_pushstring(L, "listdir failed");
            return 2;
        }
        iter = listGetIterator(queue, AL_START_HEAD);

        while ((node=listNext(iter)) != NULL) {
            const char *filename = (char *) (node->value);
            sdsclear(relpath);
            relpath = sdscatprintf(relpath, "%s/%s",(char *)head->value, filename);
            if (xfileisdir(relpath) == UGOK) {
                listAddNodeTail(dirqueue, xstrdup(relpath));
                continue;
            }
            if (match && !xstrmatch(match, filename, 0)) {
                continue;
            }
            lua_pushinteger(L, idx);
            lua_pushstring(L, relpath);
            lua_settable(L, -3);
            idx++;
        }
        listReleaseIterator(iter);
        listRelease(queue);
        listDelNode(dirqueue, head);
    }
    sdsfree(relpath);
    listRelease(dirqueue);
    return 1;
}

