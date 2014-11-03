#include "core_declarer.h"
#include "utils/uuid.h"

int core_uuid(lua_State *L) {
    char uuidbyte[38];
    if (uuidgen(uuidbyte) != UGOK) {
        lua_pushnil(L);
        lua_pushstring(L, "generate uuid failed");
        return 2;
    }
    lua_pushstring(L, uuidbyte);
    return 1;
}

