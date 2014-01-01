#include "core_declarer.h"
#include "utils/md5.h"
#include "utils/string.h"

int core_md5(lua_State* L)
{
    int top = lua_gettop(L);
    int len = 0;
    const char* str=  NULL;
    unsigned char digest[16]= {0};
    char md5[33]= {0};
    md5_state_t status;
    memset(&status, 0, sizeof(md5_state_t));
    str=luaL_checkstring(L, 1);

    if (top == 2) {
        len=luaL_checkinteger(L, 2);
    } else len = strlen(str);
    md5_init(&status);
    md5_append(&status, (md5_byte_t*)str, len);
    md5_finish(&status, (unsigned char*)digest);
    xstrdigest_convert(digest, 16, md5, 33);
    lua_pushstring(L, md5);
    return 1;

}

