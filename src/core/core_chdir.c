#include "core_declarer.h"
#include "utils/path.h"

int core_chdir(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);

    if (xchdir(path) != UGOK) {
        lua_pushnil(L);
        lua_pushfstring(L, "unable to switch to directory '%s'", path);
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}
