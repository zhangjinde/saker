
#include "ulualib.h"
#include "sysinfo.h"


static const luaL_Reg sysinfolib[] = {
    { "sysinfo",        sysInfo},
    {NULL, NULL}
};

//
//LUAMOD_API int luaopen_sysinfo(lua_State *L) {
//    luaL_newlib(L, sysinfolib);
//    return 1;
//}


const luaL_Reg *getSysinfoReg(void) {
    return sysinfolib;
}
