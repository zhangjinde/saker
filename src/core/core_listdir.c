#include "core_declarer.h"
#include "utils/file.h"

int core_listdir(lua_State *L) {
    int top = lua_gettop(L);
    const char *path =  lua_tostring(L, 1);
    const char *match = NULL;
    list *queue = listCreate();
    int idx = 1;
    listNode *node=NULL;
    listIter *iter = NULL;

    if (top == 2) {
        match = lua_tostring(L, 2);
    }
    
    if (xfilelistdir(path, match, queue) == UGERR) {
        listRelease(queue);
        lua_pushnil(L);
        lua_pushstring(L, "listdir failed");
        return 2;
    }

    iter = listGetIterator(queue, AL_START_HEAD);
    lua_createtable(L, listLength(queue), 0);
    while ((node=listNext(iter))!=NULL) {
        const char *filename = (char *) (node->value);
        lua_pushinteger(L, idx);
        lua_pushstring(L, filename);
        lua_settable(L, -3);
        idx++;
    }

    listReleaseIterator(iter);
    listRelease(queue);
    return 1;
}

