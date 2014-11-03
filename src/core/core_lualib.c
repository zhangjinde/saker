#include "ulualib.h"

#include "core_declarer.h"


static const luaL_Reg corelib[] = {
    { "adopt",       core_adopt},
    { "popen",       core_popen },
    { "exec",        core_exec},
    { "pidof",       core_pidof },
    { "kill",        core_kill},

    { "sleep",       core_sleep},

    { "uname",       core_type},
    { "osname",      core_osname},

    { "cd",          core_chdir },
    { "cp",          core_copyfile },
    { "pwd",         core_getcwd   },

    { "isdir",       core_isdir   },
    { "isfile",      core_isfile   },
    { "islink",      core_islink   },
    { "fileinfo",    core_fileinfo },

    { "ls" ,         core_listdir },
    { "walk",        core_walk },
    { "mkdir",       core_mkdir   },
    { "rmdir",       core_rmdir   },
    { "log",         core_log },
    { "uuid",        core_uuid   },
    { "md5",         core_md5},

    {NULL, NULL}
};


/*
LUAMOD_API int luaopen_core(lua_State *L) {
    luaL_newlib(L, corelib);
    return 1;
}
*/

const luaL_Reg *getCoreReg() {
    return corelib;
}
