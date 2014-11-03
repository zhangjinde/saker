#include "core_declarer.h"
#include "utils/thread.h"

int core_sleep(lua_State *L) {
    int tm = luaL_checkinteger(L, 1);

    xsleep(tm);

    return 0;
}

