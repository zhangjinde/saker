#include "core_declarer.h"


int core_islink(lua_State* L)
{
    const char* path = luaL_checkstring(L, 1);
#ifdef OS_WIN
    /** have a bug? **/
    DWORD attr;
    attr = GetFileAttributesA(path);
    lua_pushboolean(L,
                    attr > 0 && (attr & FILE_ATTRIBUTE_REPARSE_POINT));
    return 1;
#else
    struct stat st;
    if (lstat(path, &st) == 0) {
        lua_pushboolean(L, S_ISLNK(st.st_mode));
        return 1;
    }
    lua_pushnil(L);
    lua_pushstring(L, "lstat call failed");
    return 2;
#endif
}

