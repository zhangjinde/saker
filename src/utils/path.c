

#include "path.h"
#include "common/common.h"
#include <string.h>

#ifdef _WIN32
static void xpathtranslate(char *value, const char sep) {
    char *ch;
    for (ch = value; *ch != '\0'; ++ch) {
        if (*ch == '/' || *ch == '\\') {
            *ch = sep;
        }
    }
}
#endif

int xchdir(const char *path) {
    int z = UGERR;
#ifdef _WIN32
    z = !SetCurrentDirectory(path);
#else
    z = chdir(path);
#endif
    return z;
}


int xisabsolutepath(const char *path) {
    if (path[0] == '/' ||
            path[0] == '\\' ||
            path[0] == '$' ||
            (path[0] == '"' && path[1] == '$') ||
            (path[0] != '\0' && path[1] == ':')
       ) {
        return UGOK;
    }
    return UGERR;
}


int xgetcwd(char *path) {
    int result = UGERR;
    if (NULL == path)
        return result;

#ifdef _WIN32
    if (GetCurrentDirectory(MAX_STRING_LEN, path) != 0) {
        xpathtranslate(path,'/');
        result = UGOK;
    }
#else
    if (getcwd(path, MAX_STRING_LEN) != NULL) {
        result = UGOK;
    }
#endif
    return result;
}


int xmkdir(const char *path) {
    int z = UGERR;
    if (path) {
#ifdef _WIN32
        z = !CreateDirectory(path, NULL);
#else
        z = (mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0);
#endif
    }
    return z;
}

int xgetapppath(char *path) {
    char *p = NULL;
#ifdef _WIN32
    DWORD len = GetModuleFileName(NULL, path, MAX_STRING_LEN);
#else
    int   len = readlink("/proc/self/exe", path, MAX_STRING_LEN);
#endif
    if (len == 0 ||
            strlen(path) == 0) {
        return UGERR;
    }
    if ((p=strrchr(path,'\\')) || (p=strrchr(path,'/'))) {
        *p = 0;
    }

    return UGOK;
}



int xchtoapppath(void) {
    char path[MAX_STRING_LEN]= {0};
    if (xgetapppath(path)==UGERR) {
        fprintf(stderr,"locate app path failed");
        return UGERR;
    }
    xchdir(path);
    return UGOK;
}

