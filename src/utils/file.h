#ifndef _FILE__H_
#define _FILE__H_

#include <stdio.h>
#include <sys/stat.h>

#include "ulist.h"

int xfileisregular(const char *filename);

int xfileisdir(const char *path) ;

int xfilewrite(const char *filename,const char *text);

int xfilelistdir(const char *dirpath,const char *match,list *queue);

int xfiledel(const char *filepath) ;

#endif
