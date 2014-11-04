#include "core_declarer.h"
#include "utils/process.h"

int core_kill(lua_State *L) {
    int pid;
    int sig = -1;
    int top = lua_gettop(L);
    if (top <= 0) {
        lua_pushnil(L);
        lua_pushstring(L, "wrong number of arguments for kill");
        return 2;
    }
    pid = luaL_checkinteger(L, 1);
    if (top == 2) {
        sig = luaL_checkinteger(L, 2);
    }

    if (pkill(pid, sig) == UGOK) {
        lua_pushboolean(L, 1);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushstring(L, "kill failed");
        return 2;
    }
}
