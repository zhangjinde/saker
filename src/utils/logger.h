#ifndef _LOG__H_
#define _LOG__H_

#define LOG_CONST_STRING(str) ""str

enum log_priority
{
    LOGFATAL = 1,   /* A fatal error. The application will most likely terminate. This is the highest priority. */
    LOGCRITICAL,    /* A critical error. The application might not be able to continue running successfully. */
    LOGERROR,       /* An error. An operation did not complete successfully, but the application as a whole is not affected. */
    LOGWARNING,     /* A warning. An operation completed with an unexpected result. */
    LOGNOTICE,      /* A notice, which is an information with just a higher priority. */
    LOGINFORMATION, /* An informational message, usually denoting the successful completion of an operation. */
    LOGDEBUG,       /* A debugging message. */
    LOGTRACE        /* A tracing message. This is the lowest priority. */
};

#define LOG_FATAL(fmt,...)  \
    logger_write(LOGFATAL,__FILE__,__LINE__,LOG_CONST_STRING(fmt),##__VA_ARGS__);

#define LOG_CRITICAL(fmt,...)  \
    logger_write(LOGCRITICAL,__FILE__,__LINE__,LOG_CONST_STRING(fmt),##__VA_ARGS__);

#define LOG_ERROR(fmt,...)  \
    logger_write(LOGERROR,__FILE__,__LINE__,LOG_CONST_STRING(fmt),##__VA_ARGS__);

#define LOG_WARNING(fmt,...)  \
    logger_write(LOGWARNING,__FILE__,__LINE__,LOG_CONST_STRING(fmt),##__VA_ARGS__);

#define LOG_NOTICE(fmt,...)  \
    logger_write(LOGNOTICE,__FILE__,__LINE__,LOG_CONST_STRING(fmt),##__VA_ARGS__);

#define LOG_INFO(fmt,...)  \
    logger_write(LOGINFORMATION,__FILE__,__LINE__,LOG_CONST_STRING(fmt),##__VA_ARGS__);

#define LOG_DEBUG(fmt,...)  \
    logger_write(LOGDEBUG,__FILE__,__LINE__,LOG_CONST_STRING(fmt),##__VA_ARGS__);

#define LOG_TRACE(fmt,...)  \
    logger_write(LOGTRACE,__FILE__,__LINE__,LOG_CONST_STRING(fmt),##__VA_ARGS__);


int  logger_open(const char* logfile,int level);

void logger_write(int level,const char* file,int line,const char *fmt, ...);

void logger_close();

#endif
