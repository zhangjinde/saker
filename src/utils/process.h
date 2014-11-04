#ifndef _DAEMONIZE__H_
#define _DAEMONIZE__H_

#include "common/types.h"

#ifdef _WIN32
int  get_procname(HANDLE hProcess, char* procname,int len)  ;
#endif


int  pidfile_create(const char* pidfile,const char* appname,pid_t pid);

void pidfile_remove(const char* pidfile);

int  pidfile_getpid(const char* pidfile,int* pid);

int  pidfile_exists(const char* pidfile);

int  pidfile_verify(const char* pidfile);

int  proc_isrunning(pid_t pid, const char* matchstr);


void daemonize();

int  pkill(pid_t pid,int sig);




#endif
