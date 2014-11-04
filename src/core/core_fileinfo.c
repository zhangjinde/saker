#include "core_declarer.h"
#include "utils/error.h"


int core_fileinfo(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    const int more_info = lua_toboolean(L, 2);
#ifndef _WIN32
    struct stat st;
#else
    struct _stat st;
#endif
    int res;

#ifndef _WIN32
    res = stat(path, &st);
#else
    res = _stat(path, &st);
#endif

    if (res) {
        lua_pushnil(L);
        lua_pushfstring(L, "stat %s failed err:%s", path, xerrmsg());
        return 2;
    }
    lua_newtable(L);

    /* is directory? */
    lua_pushboolean(L,
#ifndef _WIN32
                    S_ISDIR(st.st_mode)
#else
                    st.st_mode & _S_IFDIR
#endif
                   );
    lua_setfield(L, -2, "IFDIR");
    /* is regular file? */
    lua_pushboolean(L,
#ifndef _WIN32
                    S_ISREG(st.st_mode)
#else
                    st.st_mode & _S_IFREG
#endif
                   );
    lua_setfield(L, -2, "IFREG");
    /* can anyone read from file? */
    lua_pushboolean(L,
#ifndef _WIN32
                    st.st_mode & (S_IRUSR | S_IRGRP | S_IROTH)
#else
                    st.st_mode & _S_IREAD
#endif
                   );
    lua_setfield(L, -2, "IFREAD");
    /* can anyone write to file? */
    lua_pushboolean(L,
#ifndef _WIN32
                    st.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)
#else
                    st.st_mode & _S_IWRITE
#endif
                   );
    lua_setfield(L, -2, "IFWRITE");
    /* can anyone execute the file? */
    lua_pushboolean(L,
#ifndef _WIN32
                    st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)
#else
                    st.st_mode & _S_IEXEC
#endif
                   );
    lua_setfield(L, -2, "IFEXEC");
    if (more_info) {
        /* is link? */
#ifndef _WIN32
        lua_pushboolean(L, S_ISLNK(st.st_mode));

#else
        DWORD attr;
        attr = GetFileAttributesA(path);
        lua_pushboolean(L,
                        attr > 0 && (attr & FILE_ATTRIBUTE_REPARSE_POINT));
#endif
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

