
#include "ulualib.h"

#include "register.h"

static const luaL_Reg svlib[] = {
    { "register",       ugRegister },
    { "publish",        ugPublish },
    { "unregister",     ugUnregister },
    {NULL,NULL}
};

//LUAMOD_API int luaopen_sv(lua_State *L) {
//    luaL_newlib(L, svlib);
//    return 1;
//}

const luaL_Reg* getSvReg()
{
    return svlib;
}
