
#include "sysinfo.h"
#include <string.h>
#include "utils/string.h"
#include "utils/udict.h"



static dict* g_dict_sysinfo_function = NULL;


/* Functions managing dictionary of callbacks for pub/sub. */
static unsigned int callbackHash(const void* key)
{
    return dictGenHashFunction((unsigned char*)key,strlen((const char*)key));
}

static void* callbackValDup(void* privdata, const void* src)
{
    ((void) privdata);
    /* readonly str ,so copy it directly*/
    return (void*)src;
}

static int callbackKeyCompare(void* privdata, const void* key1, const void* key2)
{
    int l1, l2;
    ((void) privdata);

    l1 = strlen((const char*)key1);
    l2 = strlen((const char*)key2);
    if (l1 != l2) return 0;
    return memcmp(key1,key2,l1) == 0;
}

static void callbackKeyDestructor(void* privdata, void* key)
{
    /* do nothing */
    ((void) privdata);
    ((void) key);
}

static void callbackValDestructor(void* privdata, void* val)
{
    ((void) privdata);
    ((void) val);
}

static dictType callbackDict = {
    callbackHash,
    NULL,
    callbackValDup,
    callbackKeyCompare,
    callbackKeyDestructor,
    callbackValDestructor
};

uint64_t bytesConvert(uint64_t value,const char* param)
{
    if (param == NULL) {
        //nothing
    } else if (0 == xstrncasecmp(param, "kb", 2)) {
        value >>= 10;
    } else if (0 == xstrncasecmp(param, "mb", 2)) {
        value >>= 20;
    } else if (0 == xstrncasecmp(param, "gb", 2)) {
        value >>= 30;
    } else if (0 == xstrncasecmp(param, "tb", 2)) {
        value >>= 40;
    }
    return value;
}

const char*  getParam(int argc,const char** argv, int pos)
{
    if (argv != NULL) {
        return argc>pos ? argv[pos] : NULL;
    }
    return NULL;
}

void    initResult(SYSINFO_RESULT* result)
{
    result->type = 0;

    result->ui64 = 0;
    result->dbl = 0;
    result->str = NULL;
    result->text = NULL;
    result->msg = NULL;
}

void    freeResult(SYSINFO_RESULT* result)
{
    UNSET_UI64_RESULT(result);
    UNSET_DBL_RESULT(result);
    UNSET_STR_RESULT(result);
    UNSET_TEXT_RESULT(result);
    UNSET_MSG_RESULT(result);
}


static const sysinfoMap sysinfo_map[] = {
    {"system.boottime",              SYSTEM_BOOTTIME},

    {"system.uptime",                SYSTEM_UPTIME},

    {"system.cpu.num.online",        SYSTEM_CPU_NUM_ONLINE},
    {"system.cpu.num.max",           SYSTEM_CPU_NUM_MAX},
    {"system.cpu.load.all",          SYSTEM_CPU_LOAD},
    {"system.cpu.util",              SYSTEM_CPU_UTIL},

    {"system.mem.total",             VM_MEMORY_TOTAL},
    {"system.mem.free",              VM_MEMORY_FREE},
    {"system.mem.buffers",           VM_MEMORY_BUFFERS},
    {"system.mem.cached",            VM_MEMORY_CACHED},
    {"system.mem.used",              VM_MEMORY_USED},
    {"system.mem.pused",             VM_MEMORY_PUSED},
    {"system.mem.available",         VM_MEMORY_AVAILABLE},
    {"system.mem.pavailable",        VM_MEMORY_PAVAILABLE},
    {"system.mem.shared",            VM_MEMORY_SHARED},

    {"system.net.if.in",             NET_IF_IN},
    {"system.net.if.out",            NET_IF_OUT},
    {"system.net.if.total",          NET_IF_TOTAL},
    {"system.net.if.collisions",     NET_IF_COLLISIONS},


    {"system.kernel.maxfiles" ,      KERNEL_MAXFILES },
    {"system.kernel.maxproc" ,       KERNEL_MAXPROC },


    {"system.fs.size.total" ,        VFS_FS_SIZE },
    {"system.fs.size.free" ,         VFS_FS_SIZE },
    {"system.fs.size.used" ,         VFS_FS_SIZE },
    {"system.fs.size.pfree" ,        VFS_FS_SIZE },
    {"system.fs.size.pused" ,        VFS_FS_SIZE },

    {"system.proc.id" ,              PROC_PID},
    {"system.proc.mem.used" ,        PROC_MEMORY_USED },
    {"system.proc.mem.pused" ,       PROC_MEMORY_PUSED },
    {"system.proc.cpu.load" ,        PROC_CPU_LOAD },
    {"system.proc.all",              PROC_STATINFO },
    /*
    {"system.kernel.maxproc" ,       KERNEL_MAXPROC },
    {"system.kernel.maxproc" ,       KERNEL_MAXPROC },
    */
    /*
    {"system.cpu.num.online",        SYSTEM_CPU_NUM_ONLINE},
    {"system.boottime",              SYSTEM_BOOTTIME},
    {"system.cpu.num.online",        SYSTEM_CPU_NUM_ONLINE},
    {"system.cpu.num.max",           SYSTEM_CPU_NUM_MAX},
    {"system.cpu.num.load",          SYSTEM_CPU_LOAD},
    {"system.mem.total",             VM_MEMORY_TOTAL},
    {"system.cpu.num.online",        SYSTEM_CPU_NUM_ONLINE}
    */
    {NULL,NULL}
};


int initSysinfoDic()
{
    int idx;
    g_dict_sysinfo_function = dictCreate(&callbackDict,NULL);
    if (g_dict_sysinfo_function == NULL) {
        return UGERR;
    }
    for (idx=0; sysinfo_map[idx].cmd ; ++idx) {
        if(sysinfo_map[idx].cmd != NULL && sysinfo_map[idx].func != NULL) {
            if(DICT_ERR==dictAdd(g_dict_sysinfo_function, sysinfo_map[idx].cmd, sysinfo_map[idx].func)) {
                return UGERR;
            }
        }
    }
    return UGOK;
}


void freeSysinfoDic()
{
    if (g_dict_sysinfo_function) {
        dictRelease(g_dict_sysinfo_function);
        g_dict_sysinfo_function = NULL;
    }
}

int sysInfo(lua_State* L)
{
    const char* cmd = NULL;
    const char** argv =  NULL;
    dictEntry* pit = NULL;
    SYSINFO_RESULT result;
    sysinfo_function_t fun = NULL;
    char errmsg[MAX_STRING_LEN]= {0};

    int top = lua_gettop(L);
    int idx ;
    int ret = 1;
    if (top<1) {
        lua_pushnil(L);
        lua_pushstring(L, "wrong param");
        ret = 2;
        goto END;
    }
    cmd = luaL_checkstring(L,1);
    if (top > 1) {
        argv = (const char**)zmalloc(sizeof(char*)*(top-1));
        for(idx=2; idx<=top; ++idx) {
            argv[idx-2] = (char*)luaL_checkstring(L,idx);
        }
    }

    if (NULL == g_dict_sysinfo_function) {
        if (UGOK!=initSysinfoDic()) {
            strcpy(errmsg, "initSysinfoDic failed");
            ret = 2;
            goto END;
        }
    }
    pit  = dictFind(g_dict_sysinfo_function, cmd);
    if (NULL==pit || NULL==pit->val) {
        strcpy(errmsg, "unknown primary parameter,can't find the function by it");
        ret = 2;
        goto END;
    }

    fun =(sysinfo_function_t)pit->val;

    initResult(&result);
    if (fun(cmd, top-1, argv, &result) != UGOK) {
        strcpy(errmsg, "execute the function failed. ");
        strcat(errmsg,GET_MSG_RESULT(&result));
        ret = 2;
        goto END;
    }

    if ( ISSET_UI64(&result) ) {
        lua_pushinteger(L,(ptrdiff_t)result.ui64);
    } else if ( ISSET_DBL(&result) ) {
        lua_pushnumber(L,result.dbl);
    } else if (ISSET_STR(&result) ) {
        lua_pushstring(L, result.str);
    } else if (ISSET_TEXT(&result) ) {
        lua_pushstring(L, result.text);
    } else if (ISSET_MSG(&result) ) {
        lua_pushstring(L, result.msg);
    } else {
        strcpy(errmsg, "execute the function success,but can not find return result.");
        ret = 2;
    }

END:
    if (argv) {
        zfree((char**)argv);
        argv = NULL;
    }
    freeResult(&result);

    if (ret == 2) {
        lua_pushnil(L);
        lua_pushstring(L,errmsg);
    }

    return ret;
}

