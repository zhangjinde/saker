#include "core_declarer.h"

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

int core_getuid(lua_State *L) {
    const char *user = NULL;
    int top = lua_gettop(L);
    struct passwd *pwd = NULL;
    if (top == 0) {
        lua_pushinteger(L, getuid());
        return 1;

    }
    if (top > 0) user = lua_tostring(L, 1);

    pwd = getpwnam(user);
    if (pwd) {
        lua_pushinteger(L, pwd->pw_uid);
        return 1;
    }
    lua_pushnil(L);
    lua_pushfstring(L, "getuid failed.may be user is not exists or .");
    return 2;
}
