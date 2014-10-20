#include "perf.h"
#ifndef _WIN32
#include <sys/time.h>
#include <sys/resource.h>
#endif
#include "logger.h"
#include "error.h"

/* This function will try to raise the max number of open files accordingly to
 * the configured max number of clients. It will also account for 32 additional
 * file descriptors as we need a few more for persistence, listening
 * sockets, log files and so forth.
 *
 * If it will not be possible to set the limit accordingly to the configured
 * max number of clients, the function will do the reverse setting
 * server.maxclients to the value that we can actually handle. */
unsigned long long adjustOpenFilesLimit(unsigned long clientsnum) 
{
    unsigned long long maxclients = clientsnum;
#ifndef _WIN32    
    rlim_t maxfiles = maxclients+32;
    struct rlimit limit;

    if (getrlimit(RLIMIT_NOFILE,&limit) == -1) {
        LOG_WARNING("Unable to obtain the current NOFILE limit (%s), assuming 1024 and setting the max clients configuration accordingly.",
           xerrmsg());
        maxclients = 1024-32;
    } else {
        rlim_t oldlimit = limit.rlim_cur;

        /* Set the max number of files if the current limit is not enough
         * for our needs. */
        if (oldlimit < maxfiles) {
            rlim_t f;
            
            f = maxfiles;
            while(f > oldlimit) {
                limit.rlim_cur = f;
                limit.rlim_max = f;
                if (setrlimit(RLIMIT_NOFILE,&limit) != -1) break;
                f -= 128;
            }
            if (f < oldlimit) f = oldlimit;
            if (f != maxfiles) {
                maxclients = f-32;
                LOG_WARNING("Unable to set the max number of files limit to %d (%s), setting the max clients configuration to %d.",
                    (int) maxfiles, xerrmsg(), (int) maxclients);
            } else {
                LOG_TRACE("Max number of open files set to %d",
                    (int) maxfiles);
            }
        }
    }
#endif
    return maxclients;
}