#include "core_declarer.h"
#include "utils/path.h"

int core_mkdir(lua_State *L) {
    int z;
    int top = lua_gettop(L);
    const char *path = NULL;
    int idx = 0;
    for ( ; idx<top; ++idx) {
        path = luaL_checkstring(L, idx+1);
        z = xmkdir(path);
        if (!z) {
            lua_pushnil(L);
            lua_pushfstring(L, "unable to create directory '%s'", path);
            return 2;
        }
    }
    lua_pushboolean(L, 1);
    return 1;
}

