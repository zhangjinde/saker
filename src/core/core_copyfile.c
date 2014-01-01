#include "core_declarer.h"


int core_copyfile(lua_State* L)
{
    int z;
    const char* src = luaL_checkstring(L, 1);
    const char* dst = luaL_checkstring(L, 2);

#if OS_WIN
    struct stat buf;
    if (stat(src, &buf) != 0) {
        lua_pushnil(L);
        lua_pushfstring(L, "unable to stat '%s'", src);
    }

    if (buf.st_mode & S_IFDIR) {

    } else {
        z = CopyFileA(src, dst, FALSE);
    }

#else
    lua_pushfstring(L, "cp -r %s %s", src, dst);
    z = (system(lua_tostring(L, -1)) == 0);
#endif

    if (!z) {
        lua_pushnil(L);
        lua_pushfstring(L, "unable to copy file to '%s'", dst);
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

