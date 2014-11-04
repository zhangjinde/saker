#include "core_declarer.h"
#include "utils/file.h"

int core_isfile(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);
    lua_pushboolean(L, UGOK == xfileisregular(filename));
    return 1;
}
