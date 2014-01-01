#ifndef _ULUALIB__H_
#define _ULUALIB__H_


#include "luaworking.h"

//#define LUA_CORELIBNAME	"saker"
//LUAMOD_API int (luaopen_core) (lua_State *L);
//
//
//#define LUA_SVLIBNAME	"saker"
//LUAMOD_API int (luaopen_sv) (lua_State *L);


typedef const luaL_Reg* (*getlib)();

const luaL_Reg* getCoreReg();

const luaL_Reg* getSvReg();

const luaL_Reg* getSysinfoReg();

#define LUA_SAKERNAME   "saker"
LUAMOD_API int (luaopen_saker) (lua_State *L);

#define LUA_JSONNAME    "json"
LUAMOD_API int (luaopen_cjson) (lua_State *L);

#define LUA_MSGPAKCNAME "msgpack"
LUAMOD_API int (luaopen_cmsgpack) (lua_State *L);

#define LUA_REDISNAME "hiredis"
LUAMOD_API int (luaopen_hiredis) (lua_State *L);

extern const luaL_Reg ulualib[];

#endif





