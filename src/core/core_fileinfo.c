#include "core_declarer.h"
#include "utils/error.h"


int core_fileinfo(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    const int more_info = lua_toboolean(L, 2);
    struct stat st;
    int res;
    res = stat(path, &st);
    if (res) {
        lua_pushnil(L);
        lua_pushfstring(L, "stat %s failed err:%s", path, xerrmsg());
        return 2;
    }
    lua_newtable(L);

    /* is directory? */
    lua_pushboolean(L,
                    S_ISDIR(st.st_mode)
                   );
    lua_setfield(L, -2, "IFDIR");
    /* is regular file? */
    lua_pushboolean(L,
                    S_ISREG(st.st_mode)
                   );
    lua_setfield(L, -2, "IFREG");
    /* can anyone read from file? */
    lua_pushboolean(L,
                    st.st_mode & (S_IRUSR | S_IRGRP | S_IROTH)
                   );
    lua_setfield(L, -2, "IFREAD");
    /* can anyone write to file? */
    lua_pushboolean(L,
                    st.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)
                   );
    lua_setfield(L, -2, "IFWRITE");
    /* can anyone execute the file? */
    lua_pushboolean(L,
                    st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)
                   );
    lua_setfield(L, -2, "IFEXEC");
    if (more_info) {
        /* is link? */
        lua_pushboolean(L, S_ISLNK(st.st_mode));
        lua_setfield(L, -2, "IFLINK");
        lua_pushnumber(L, (lua_Number) st.st_size);  /* size in bytes */
        lua_setfield(L, -2, "SIZE");
        lua_pushnumber(L, (lua_Number) st.st_atime);  /* access time */
        lua_setfield(L, -2, "ATIME");
        lua_pushnumber(L, (lua_Number) st.st_mtime);  /* modification time */
        lua_setfield(L, -2, "MTIME");
        lua_pushnumber(L, (lua_Number) st.st_ctime);  /* creation time */
        lua_setfield(L, -2, "CTIME");
    }
    return 1;
}
