#include "core_declarer.h"
#include "utils/path.h"


int core_getcwd(lua_State *L) {
    char buffer[MAX_STRING_LEN] = {0};
    char *ch;
    if(xgetcwd(buffer) == UGERR) {
        lua_pushnil(L);
        lua_pushstring(L, "call getcwd failed");
        return 2;
    }

    /* convert to platform-neutral directory separators */
    for (ch = buffer; *ch != '\0'; ++ch) {
        if (*ch == '\\') *ch = '/';
    }

    lua_pushstring(L, buffer);
    return 1;
}


