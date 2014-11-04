#ifndef _PATH__H_
#define _PATH__H_

int xchdir(const char* path);

int xisabsolutepath(const char* path) ;

int xgetcwd(char* path);

int xmkdir(const char* path);

int xgetapppath(char* path);

int xchtoapppath(void) ; 

#endif
