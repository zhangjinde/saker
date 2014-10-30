

#include "logger.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>  /* for getpid */
#include <unistd.h>    /* for getpid */

#include "common/defines.h"
#include "common/common.h"
#include "utils/path.h"
#include "utils/error.h"


static const char* const log_array[]= {"[FATAL]",
                                       "[CRITICAL]",
                                       "[ERROR]",
                                       "[WARNING]",
                                       "[NOTICE]",
                                       "[INFO]",
                                       "[DEBUG]",
                                       "[TRACE]"
                                      };


static int   loglv    = LOGTRACE;
static const  char* const defaultfilename =  APPNAME".log";
static char* logfilename =  NULL    ;

int logger_open(const char* logfile, int level)
{
    FILE* logfp = fopen(logfile, "a");
    int len = strlen(logfile) + 1;
    char buff[MAX_STRING_LEN]= {0};
    if (logfp == NULL) {
        fprintf(stderr, "open log file %s failed",logfile);
        return UGERR;
    }
    fclose(logfp);

    loglv = level;

    if ((NULL==logfilename) || (defaultfilename == logfilename)) {
        /* convert relative path to absolute path */
        if (UGOK != xisabsolutepath(logfile)) {
            if(UGOK == xgetcwd(buff)) {
                len+=strlen(buff) + 1;
                strcat(buff, "/");
            }
        }

        logfilename = (char*) zmalloc(len);
        memset(logfilename, 0, len);
        strcpy(logfilename, buff);
        strcat(logfilename, logfile);
    }
    LOG_NOTICE("logger start.");
    return UGOK;
}

void logger_write(int level, const char* file, int line, const char* fmt, ...)
{
    char buffer[128]= {0};
    FILE* logfp = NULL;
    va_list vl;
    time_t now ;
    struct tm* ptm = NULL;

    if (level > loglv ) {
        return;
    }
    if (NULL == logfilename) {
        logfilename = (char*) defaultfilename;
    }
    logfp = fopen(logfilename, "a");
    if (logfp == NULL) {
        fprintf(stderr, "open log file %s failed %s", logfilename, xerrmsg());
        return;
    }

    time(&now);
    ptm = localtime(&now);

    snprintf(buffer, 128, "[%04d-%02d-%02d %02d:%02d:%02d]",
             ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
             ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

    fprintf(logfp, "%s %s [%d] [%s:%d] ",buffer, log_array[level-1], getpid(), file, line);

    va_start(vl,fmt);
    vfprintf(logfp,fmt,vl);
    va_end(vl);
    fprintf(logfp,ENDFLAG);

    if (fclose(logfp)) {
        fprintf(stderr, "close %s failed %s", logfilename, xerrmsg());
    }
}

void logger_close()
{
    if ((logfilename != NULL) &&
            (logfilename != defaultfilename)) {
        zfree(logfilename);
    }
}
