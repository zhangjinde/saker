#include  "file.h"
#include <string.h>
#include "common/common.h"
#include "string.h"


#ifdef OS_UNIX
#include <sys/types.h>
#include <dirent.h>
#endif

static void freebuff(void* p)
{
    char* buff = (char*)p;
    if (buff) {
        zfree(buff);
        buff = NULL;
    }
}


int xfileisregular(const char* filename)
{
    struct stat buf;
    if (stat(filename, &buf) == 0) {
        if (buf.st_mode & S_IFREG) return UGOK;
    }
    return UGERR;
}

int xfileisdir(const char* path)
{
    struct stat buf;
    if (stat(path, &buf) == 0) {
        if (buf.st_mode & S_IFDIR) return UGOK;
    }
    return UGERR;
}


int xfilewrite(const char* filename, const char* text)
{
    FILE* fp = fopen(filename,"wb");
    if (fp) {
        fwrite(text,strlen(text),1,fp);
        fclose(fp);
        return UGOK;
    }
    return UGERR;
}

int xfilelistdir(const char* dirpath, const char* match, list* queue)
{
    char* path = NULL;
#ifdef OS_WIN
    HANDLE          fh;
    WIN32_FIND_DATA fd;
    char            pathstr[MAX_STRING_LEN]= {0};

    if (queue == NULL) {
        return UGERR;
    }
    listSetFreeMethod(queue, freebuff);
    strcpy(pathstr,dirpath);
    strcat(pathstr,"/*");
    fh = FindFirstFile(pathstr, &fd);
    do {
        if (fh == INVALID_HANDLE_VALUE)
            break;
        if (strcmp(fd.cFileName, ".")==0 || strcmp(fd.cFileName, "..")==0)
            continue;
        if (match && !xstrmatch(match,fd.cFileName,0))
            continue;
        path = xstrdup(fd.cFileName);
        listAddNodeTail(queue,path);
    } while ((FindNextFile(fh, &fd) != 0));
    FindClose(fh);
#else
    DIR* pDir = opendir(dirpath);
    struct dirent* pEntry = NULL;
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
#endif
    return UGOK;

}


int xfiledel(const char* filepath)
{
    if (!filepath) return UGERR;
#ifdef  OS_WIN
    if ( _unlink(filepath) == -1 ) {
        return UGERR;
    }
#else
    if (unlink(filepath) != 0) {
        return UGERR;
    }
#endif

    return UGOK;


}
