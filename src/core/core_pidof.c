
#include "core_declarer.h"

#include "utils/process.h"

int core_pidof(lua_State *L) {
    const char *path = NULL, *key= NULL ;
    int pid;
    int top = lua_gettop(L);
    if (top == 0) {
        lua_pushnil(L);
        lua_pushfstring(L, "wrong number of arguments for '%s'", "core_pidof");
        return 2;
    }
    if (top > 0) path = lua_tostring(L, 1);
    if (top > 1) key  = lua_tostring(L, 2);

    if (pidfile_exists(path) == UGERR) {
        lua_pushnil(L);
        lua_pushfstring(L, "cannot find the pidfile '%s'", path);
        return 2;
    }
    if (pidfile_getpid(path, &pid) == UGERR) {
        lua_pushnil(L);
        lua_pushfstring(L, "get pid failed by pidfile '%s'", path);
        return 2;
    }
    if (pidfile_verify(path) == UGOK ||
            (key != NULL && proc_isrunning(pid, key) == UGOK)) {
        lua_pushinteger(L, pid);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

