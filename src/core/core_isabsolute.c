#include "core_declarer.h"
#include "utils/path.h"

int core_isabsolutepath(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);

    if (xisabsolutepath(path) == UGOK) {
        lua_pushboolean(L, 1);
        return 1;
    }
    return 0;
}
