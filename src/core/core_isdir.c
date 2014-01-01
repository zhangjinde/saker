#include "core_declarer.h"
#include "utils/file.h"

int core_isdir(lua_State* L)
{
    const char* path = luaL_checkstring(L, 1);

    if (xfileisdir(path) == UGOK)  lua_pushboolean(L, 1);
    else  lua_pushboolean(L, 0);
    return 1;
}

