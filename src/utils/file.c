#include  "file.h"
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include "common/common.h"
#include "string.h"

static void freebuff(void *p) {
    char *buff = (char *)p;
    if (buff) {
        zfree(buff);
        buff = NULL;
    }
}

int xfileisregular(const char *filename) {
    struct stat buf;
    if (stat(filename, &buf) == 0) {
        if (buf.st_mode & S_IFREG) return UGOK;
    }
    return UGERR;
}

int xfileisdir(const char *path) {
    struct stat buf;
    if (stat(path, &buf) == 0) {
        if (buf.st_mode & S_IFDIR) return UGOK;
    }
    return UGERR;
}

int xfilewrite(const char *filename, const char *text) {
    FILE *fp = fopen(filename,"wb");
    if (fp) {
        fwrite(text,strlen(text),1,fp);
        fclose(fp);
        return UGOK;
    }
    return UGERR;
}

int xfilelistdir(const char *dirpath, const char *match, list *queue) {
    char *path = NULL;
    DIR *pDir = opendir(dirpath);
    struct dirent *pEntry = NULL;
    if (pDir == NULL) {
        return UGERR;
    }
    if (queue == NULL) {
        return UGERR;
    }
    listSetFreeMethod(queue, freebuff);
    while (NULL!=(pEntry=readdir(pDir))) {
        if (strcmp(pEntry->d_name, ".")==0 || strcmp(pEntry->d_name, "..")==0)
            continue;
        if (match && !xstrmatch(match, pEntry->d_name,0))
            continue;
        path=xstrdup(pEntry->d_name);
        listAddNodeTail(queue, path);
    }

    if (pDir) {
        closedir(pDir);
    }
    return UGOK;

}


int xfiledel(const char *filepath) {
    if (!filepath) return UGERR;
    
    if (unlink(filepath) != 0) {
        return UGERR;
    }
    return UGOK;
}


