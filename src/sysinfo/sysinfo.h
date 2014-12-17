
#ifndef _SYSINFO__H_
#define _SYSINFO__H_

#include "common/common.h"
#include "common/types.h"
#include "luaworking.h"

typedef struct {
    int         type;
    uint64_t    ui64;
    double      dbl;
    char        *str;
    char        *text;
    char        *msg;
} SYSINFO_RESULT;

/* agent result types */
#define AR_UINT64   0x01
#define AR_DOUBLE   0x02
#define AR_STRING   0x04
#define AR_TEXT     0x08
#define AR_MESSAGE  0x10

/* SET RESULT */

#define SET_UI64_RESULT(res, val)   \
    (   \
    (res)->type |= AR_UINT64,   \
    (res)->ui64 = (uint64_t)(val)   \
    )

#define SET_DBL_RESULT(res, val)    \
    (   \
    (res)->type |= AR_DOUBLE,   \
    (res)->dbl = (double)(val)  \
    )

/* NOTE: always allocate new memory for val! DON'T USE STATIC OR STACK MEMORY!!! */
#define SET_STR_RESULT(res, val)    \
    (   \
    (res)->type |= AR_STRING,   \
    (res)->str = (char *)(val)  \
    )

/* NOTE: always allocate new memory for val! DON'T USE STATIC OR STACK MEMORY!!! */
#define SET_TEXT_RESULT(res, val)   \
    (   \
    (res)->type |= AR_TEXT, \
    (res)->text = (char *)(val) \
    )

/* NOTE: always allocate new memory for val! DON'T USE STATIC OR STACK MEMORY!!! */
#define SET_MSG_RESULT(res, val)    \
    (   \
    (res)->type |= AR_MESSAGE,  \
    (res)->msg = (char *)(val)  \
    )

/* CHECK RESULT */

#define ISSET_UI64(res) ((res)->type & AR_UINT64)
#define ISSET_DBL(res)  ((res)->type & AR_DOUBLE)
#define ISSET_STR(res)  ((res)->type & AR_STRING)
#define ISSET_TEXT(res) ((res)->type & AR_TEXT)
#define ISSET_MSG(res)  ((res)->type & AR_MESSAGE)

/* UNSET RESULT */

#define UNSET_UI64_RESULT(res)  \
    (   \
    (res)->type &= ~AR_UINT64,  \
    (res)->ui64 = (uint64_t)0   \
    )

#define UNSET_DBL_RESULT(res)   \
    (   \
    (res)->type &= ~AR_DOUBLE,  \
    (res)->dbl = (double)0  \
    )

#define UNSET_STR_RESULT(res)   \
    \
    do  \
{   \
    if ((res)->type & AR_STRING)    \
{   \
    zfree((res)->str);  \
    (res)->type &= ~AR_STRING;  \
}   \
}       \
    while (0)

#define UNSET_TEXT_RESULT(res)  \
    \
    do                      \
{                       \
    if ((res)->type & AR_TEXT)  \
{                   \
    zfree((res)->text);     \
    (res)->type &= ~AR_TEXT;    \
}   \
}   \
    while (0)

#define UNSET_MSG_RESULT(res)   \
    \
    do  \
{   \
    if ((res)->type & AR_MESSAGE)   \
{   \
    zfree((res)->msg);  \
    (res)->type &= ~AR_MESSAGE; \
}   \
}   \
    while (0)

#define UNSET_RESULT_EXCLUDING(res, exc_type)   \
    \
    do  \
{       \
    if (!(exc_type & AR_UINT64))    UNSET_UI64_RESULT(res); \
    if (!(exc_type & AR_DOUBLE))    UNSET_DBL_RESULT(res);  \
    if (!(exc_type & AR_STRING))    UNSET_STR_RESULT(res);  \
    if (!(exc_type & AR_TEXT))      UNSET_TEXT_RESULT(res); \
    if (!(exc_type & AR_MESSAGE))   UNSET_MSG_RESULT(res);  \
}   \
    while (0)


#define GET_UI64_RESULT(res)    \
    (((res)->type & AR_UINT64) ? (res)->ui64 : 0)

#define GET_STR_RESULT(res) \
    (((res)->type & AR_STRING) ? (res)->str : "") 

#define GET_DBL_RESULT(res) \
    (((res)->type & AR_DOUBLE) ? (res)->dbl : 0.0); 

#define GET_TEXT_RESULT(res) \
    (((res)->type & AR_TEXT) ? (res)->text : "") 

#define GET_MSG_RESULT(res) \
    (((res)->type & AR_MESSAGE) ? (res)->msg : "") 

void  initResult(SYSINFO_RESULT *result);

void  freeResult(SYSINFO_RESULT *result);

const char *getParam(int argc, const char **param, int pos) ;

uint64_t  bytesConvert(uint64_t value, const char *param) ;

int sysInfo(lua_State *L) ;

int initSysinfoDic(void);

void freeSysinfoDic(void);

typedef int (*sysinfo_function_t)(const char *, int , const char **, SYSINFO_RESULT *) ;

typedef struct {
    char *cmd;
    sysinfo_function_t func;
}
sysinfoMap;


int SYSTEM_BOOTTIME(const char *cmd,int argc,const char **argv, SYSINFO_RESULT *result);

int SYSTEM_CPU_NUM_ONLINE(const char *cmd, int argc, const char **argv, SYSINFO_RESULT *result);

int SYSTEM_CPU_NUM_MAX(const char *cmd, int argc, const char **argv, SYSINFO_RESULT *result);

int SYSTEM_CPU_LOAD(const char *cmd, int argc, const char **argv, SYSINFO_RESULT *result);

int SYSTEM_CPU_UTIL(const char *cmd, int argc, const char **argv, SYSINFO_RESULT *result);

int SYSTEM_UPTIME(const char *cmd, int argc,const char **argv, SYSINFO_RESULT *result);

int VM_MEMORY_TOTAL(const char *cmd, int argc,const char **argv, SYSINFO_RESULT *result);

int VM_MEMORY_FREE(const char *cmd, int argc,const char **argv, SYSINFO_RESULT *result);

int VM_MEMORY_BUFFERS(const char *cmd, int argc, const char **argv, SYSINFO_RESULT *result);

int VM_MEMORY_CACHED(const char *cmd, int argc,const char **argv, SYSINFO_RESULT *result);

int VM_MEMORY_USED(const char *cmd, int argc, const char **argv, SYSINFO_RESULT *result);

int VM_MEMORY_PUSED(const char *cmd, int argc,const char **argv, SYSINFO_RESULT *result);

int VM_MEMORY_AVAILABLE(const char *cmd, int argc,const char **argv, SYSINFO_RESULT *result);

int VM_MEMORY_PAVAILABLE(const char *cmd, int argc,const char **argv, SYSINFO_RESULT *result);

int VM_MEMORY_SHARED(const char *cmd, int argc, const char ** argv, SYSINFO_RESULT *result);

int NET_IF_IN(const char *cmd, int argc, const char** argv, SYSINFO_RESULT *result);

int NET_IF_OUT(const char*cmd, int argc, const char** argv, SYSINFO_RESULT *result);

int NET_IF_TOTAL(const char *cmd, int argc, const char** argv, SYSINFO_RESULT *result);

int NET_IF_COLLISIONS(const char *cmd, int argc, const char **argv, SYSINFO_RESULT *result);


int KERNEL_MAXFILES(const char *cmd, int argc, const char **argv, SYSINFO_RESULT *result);

int KERNEL_MAXPROC(const char *cmd, int argc, const char **argv, SYSINFO_RESULT *result);


int VFS_FS_SIZE(const char *cmd, int argc, const char **argv, SYSINFO_RESULT *result);


int PROC_PID(const char *cmd, int argc, const char **argv, SYSINFO_RESULT *result);

int PROC_MEMORY_RSS(const char *cmd, int argc, const char **argv, SYSINFO_RESULT *result);

int PROC_MEMORY_USED(const char *cmd, int argc, const char **argv, SYSINFO_RESULT *result);

int PROC_MEMORY_PUSED(const char *cmd, int argc, const char **argv, SYSINFO_RESULT *result);

int PROC_CPU_LOAD(const char *cmd, int argc, const char **argv, SYSINFO_RESULT *result);

int PROC_STATINFO(const char *cmd, int argc, const char **argv, SYSINFO_RESULT *result);

#endif
