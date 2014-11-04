#include "core_declarer.h"

int core_type(lua_State *L) {
    lua_pushstring(L, OS_TYPE);
    return 1;
}

int core_osname(lua_State *L) {
    lua_pushstring(L, OS_STRING);
    return 1;
}

