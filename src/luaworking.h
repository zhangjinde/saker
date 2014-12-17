
#ifndef _LUA_WRAPPER__H_
#define _LUA_WRAPPER__H_

#include "luacompat/luacompat52.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "common/common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef int   LUA_HANDLE ;

#define LUAWORK_ERR_LEN 256
/**
 * open lua state
 */
lua_State* luaworkOpen(void);


/**
 * close lua state
 */
void luaworkClose(lua_State *L);

/* 
* setenv 
*/
void luaworkSetEnv(lua_State *L, const char *key, const char *value);

/* 
* getenv 
*/
const char* luaworkGetEnv(lua_State *L, const char *key);

/**
 *  load inner lua lib 
 */
int  luaworkRefLib(lua_State *L, const luaL_Reg *reg, char *err);


/**
 *  execute lua string 
 */
int  luaworkDoString(lua_State *L, const char *code, char *err);


/**
 * execute lua file
 */
int  luaworkDoFile(lua_State *L, const char *path,  char *err);


/**
 * execute all file in dir xx
 */
int  luaworkDoDir(lua_State *L, const char *path,  char *err);


LUA_HANDLE luaworkRefFunction(lua_State *L, const char *func, char *err);


void luaworkUnrefFunction(lua_State *L, LUA_HANDLE handle, char *err);


int luaworkInnerCall(lua_State *L, char *err, const char *sig, va_list vl);

/**
 * howtouse:
 * eg:call_luamethod(L,"add","ii>i",2,3,&value);
 * b (&int) -- bool  1-true 0-false
 * i (&int)-- integer
 * d (&double)-- double
 * s (&ptr) -- string
 */
int luaworkCallByName(lua_State *L, const char *func, char *err, const char *sig, ...);


int luaworkCallByRef(lua_State *L, LUA_HANDLE handle, char *err, const char *sig, ...);


void luaworkTraceStack(lua_State *L, int n, char *msg, size_t msglen);



#endif
