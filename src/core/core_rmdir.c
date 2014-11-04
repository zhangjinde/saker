#include "core_declarer.h"
#include <stdlib.h>

int core_rmdir(lua_State *L) {
    int z;
    int top = lua_gettop(L);

    const char *path =  NULL;
    int idx = 0;

    for (; idx < top; ++idx) {
        path = luaL_checkstring(L, idx+1);
#if OS_WIN
        /* Nonzero if successful; otherwise 0. */
        z = RemoveDirectory(path);
#else
        /* 0 success  ,oterwise failed */
        z = (rmdir(path) == 0);
#endif
        if (!z) {
            lua_pushnil(L);
            lua_pushfstring(L, "unable to remove directory '%s'", path);
            return 2;
        }
    }

    lua_pushboolean(L, 1);
    return 1;
}

