
#ifndef _CORE_DECLARER__H_
#define _CORE_DECLARER__H_

#include "common/common.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "utils/logger.h"



/* Built-in functions */
int core_adopt(lua_State* L);
int core_popen(lua_State* L);
int core_exec(lua_State* L);
int core_kill(lua_State* L);
int core_sleep(lua_State* L);
int core_type(lua_State* L);
int core_osname(lua_State* L);
int core_chdir(lua_State* L);

int core_getcwd(lua_State* L);

int core_isdir(lua_State* L);
int core_isfile(lua_State* L);
int core_islink(lua_State* L);
int core_fileinfo(lua_State* L);

int core_isabsolutepath(lua_State* L);

int core_walk(lua_State* L);
int core_copyfile(lua_State* L);
int core_listdir(lua_State* L);

int core_mkdir(lua_State* L);
int core_rmdir(lua_State* L);
int core_uuid(lua_State* L);
int core_pidof(lua_State* L);
int core_log(lua_State* L);
int core_md5(lua_State* L);




#endif
