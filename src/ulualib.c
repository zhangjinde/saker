#include "ulualib.h"
#include "common/common.h"
#include <string.h>
#include <stdlib.h>

static getlib baselibs[] = {
    getCoreReg,
    getSvReg,
    getSysinfoReg,
    NULL
};

LUAMOD_API int luaopen_saker(lua_State *L) {
    getlib *mylib = baselibs;
    luaL_Reg *array = NULL;
    int len = 0;
    for (mylib=baselibs; *mylib; ++mylib) {
        const luaL_Reg *lib ;
        int idx = 0;
        for (lib = (*mylib)(); lib->func; ++lib) {
            ++idx;
        }
        array = zrealloc(array, sizeof(luaL_Reg)*(len+idx));
        memcpy(array+len, (*mylib)(), sizeof(luaL_Reg)*idx);
        len += idx;
    }
    array = zrealloc(array,sizeof(luaL_Reg)*(len+1));
    array[len].name = NULL;
    array[len].func = NULL;
    luaL_newlibx(L, array);
    zfree(array);
    return 1;
}


const luaL_Reg ulualib[] = {
    {LUA_SAKERNAME,  luaopen_saker},
    {LUA_JSONNAME ,  luaopen_cjson},
    {LUA_MSGPAKCNAME,luaopen_cmsgpack},
    {LUA_REDISNAME,  luaopen_hiredis},
    {NULL,NULL}
};
