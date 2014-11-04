#include "core_declarer.h"
#include "utils/error.h"
#include "utils/sds.h"

int core_popen(lua_State *L) {
    FILE *fp = NULL;
    char buff[128] = {0};
    sds s = sdsempty();
    const char *cmd = luaL_checkstring(L, 1);

    if( (fp = popen(cmd, "r" )) == NULL ) {
        lua_pushnil(L);
        lua_pushfstring(L, "popen failed %s", xerrmsg());
        return 2;
    }
    while( !feof( fp ) ) {
        if( fgets( buff, 128, fp ) != NULL )
            s = sdscat(s, buff);
    }
    lua_pushstring(L, s);
    sdsfree(s);
    if (pclose(fp) != 0) {
        /* todo here */
    }
    return 1;
}
