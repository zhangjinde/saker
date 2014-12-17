#include "core_declarer.h"
#include "utils/sds.h"
#include "luaworking.h"

int core_log(lua_State *L) {
    int idx = 1;
    int loglevel = 0;
    sds msg;
    int top = lua_gettop(L);
    if ( top < 2) {
        lua_pushnil(L);
        lua_pushstring(L, "log() requires two arguments or more.");
        return 2;
    }
    if (!lua_isnumber(L,-top)) {
        lua_pushnil(L);
        lua_pushstring(L, "First argument must be a integer (log level).");
    }

    loglevel = lua_tointeger(L, -top);

    /* Glue together all the arguments */
    msg = sdsempty();
    for (idx = 1; idx < top; idx++) {
        size_t len;
        char *s = (char *)lua_tolstring(L, (-top)+idx, &len);
        if (s) {
            if (idx != 1) msg = sdscatlen(msg," ",1);
            msg = sdscatlen(msg, s, len);
        }
    }
    if (loglevel <= LOGERROR) {
        char buf[MAX_STRING_LEN] = {0};
        luaworkTraceStack(L, 1, buf, MAX_STRING_LEN);
        logger_write(loglevel, __FILE__, __LINE__,"[LUA] %s. \n%s", msg, buf);
    } else {
        logger_write(loglevel, __FILE__, __LINE__,"[LUA] %s",msg);
    }
    sdsfree(msg);

    return 0;
}

