#include "path.h"
#include "common/common.h"
#include <string.h>

int xchdir(const char *path) {
    int z = UGERR;
    z = chdir(path);
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
    if (getcwd(path, MAX_STRING_LEN) != NULL) {
        result = UGOK;
    }
    return result;
}


int xmkdir(const char *path) {
    int z = UGERR;
    if (path) {
        z = (mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0);
    }
    return z;
}

int xgetapppath(char *path) {
    char *p = NULL;
    int   len = readlink("/proc/self/exe", path, MAX_STRING_LEN);
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

