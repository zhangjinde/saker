#include "config.h"
#include "common/common.h"
#include "utils/path.h"
#include "utils/file.h"
#include "utils/string.h"
#include "utils/sds.h"
#include "utils/logger.h"
#include "common/defines.h"

static const char* default_configfile="config";


/**
 *   for init config
 */
static const char* luacode = LOGFILE_PATH"   ./"APPNAME".log"ENDFLAG
                             LOGFILE_LEVEL"  8"ENDFLAG
                             SCRIPT_DIR"  ../script"ENDFLAG
                             PIDFILE_DIR"  ../pid"ENDFLAG
                             WORK_INTERVAL"   5000 "ENDFLAG
                             BIND"    127.0.0.1"ENDFLAG
                             LOCAL_PORT"   12007"ENDFLAG
                             TOP_MODE" 3000"ENDFLAG
                             MAXCLIENTS" 1000"ENDFLAG
                             "#"PASSWORD" qazplm"ENDFLAG 
                             ;


static void parse_string(config_t* config, char* configstr)
{
    char* err = NULL;
    int linenum = 0, totlines, i;
    sds* lines;

    lines = sdssplitlen(configstr, strlen(configstr), "\n", 1, &totlines);

    for (i = 0; i < totlines; i++) {
        sds* argv;
        int argc;

        linenum = i+1;
        lines[i] = sdstrim(lines[i], " \t\r\n");

        /* Skip comments and blank lines */
        if (lines[i][0] == '#' || lines[i][0] == '\0') continue;

        /* Split into arguments */
        argv = sdssplitargs(lines[i], &argc);
        if (argv == NULL) {
            err = "Unbalanced quotes in configuration line";
            goto loaderr;
        }

        /* Skip this line if the resulting command vector is empty. */
        if (argc == 0) {
            sdsfreesplitres(argv, argc);
            return;
        }
        sdstolower(argv[0]);

        /* Execute config directives */
        if (!xstrcasecmp(argv[0], LOGFILE_PATH) && argc == 2) {
            zfree((config->logfile_path));
            config->logfile_path = xstrdup(argv[1]);
        } else if (!xstrcasecmp(argv[0], LOGFILE_LEVEL) && argc == 2) {
            config->logfile_level = atoi(argv[1]);
            if (config->logfile_level < 0 || config->logfile_level > 8) {
                err = "Invalid loglevel";
                goto loaderr;
            }
        } else if (!xstrcasecmp(argv[0], WORK_INTERVAL) && argc == 2) {
            config->work_interval = atoi(argv[1]);
            if (config->work_interval < 0 ) {
                err = "Invalid work_interval";
                goto loaderr;
            }
        } else if (!xstrcasecmp(argv[0], SCRIPT_DIR) && argc == 2) {
            zfree(config->script_dir);
            config->script_dir=xstrdup(argv[1]);
        } else if (!xstrcasecmp(argv[0], PIDFILE_DIR) && argc == 2) {
            zfree(config->pidfile_dir);
            config->pidfile_dir=xstrdup(argv[1]);
        } else if (!xstrcasecmp(argv[0], TOP_MODE) && argc == 2) {
            /* default top_mode = 0 ,that means off mode */
            config->top_mode = atoi(argv[1]);;
        }
        //else if (!xstrcasecmp(argv[0],NET_TIMEOUT) && argc == 2) {
        //config->net_timeout = atoi(argv[1]);
        //if (config->net_timeout < 0 ) {
        //  err = "Invalid net_timeout"; goto loaderr;
        //}
        //}
        else if (!xstrcasecmp(argv[0], BIND) && argc == 2) {
            zfree(config->bind);
            config->bind = xstrdup(argv[1]);
        }else if (!xstrcasecmp(argv[0], LOCAL_PORT) && argc == 2) {
            config->port = atoi(argv[1]);
            if (config->port < 0 || config->port > 65535) {
                err = "Invalid port";
                goto loaderr;
            }
        }else if (!xstrcasecmp(argv[0], MAXCLIENTS) && argc == 2) {
            config->maxclients = atoi(argv[1]);
            if (config->port < 0) {
                err = "Invalid "MAXCLIENTS;
                goto loaderr;
            }
        } else if (!xstrcasecmp(argv[0], PASSWORD) && argc == 2) {
            zfree(config->password);
            config->password = xstrdup(argv[1]);
        }
        sdsfreesplitres(argv,argc);
    }
    sdsfreesplitres(lines,totlines);
    return;
loaderr:
    fprintf(stderr, "\n*** FATAL CONFIG FILE ERROR ***\n");
    fprintf(stderr, "Reading the configuration file, at line %d\n", linenum);
    fprintf(stderr, ">>> '%s'\n", lines[i]);
    fprintf(stderr, "%s\n", err);
    exit(1);

}


config_t*  createConfig(const char* filename )
{
    config_t* pconfig = zmalloc(sizeof(config_t));
    sds config = sdsempty();
    char buf[MAX_STRING_LEN+1];
    memset(pconfig,0,sizeof(config_t));
    if (NULL==filename) {
        filename = default_configfile;
    }
    if (UGERR == xfileisregular(filename)) {
        xfilewrite(filename,luacode);
    }
    /* Load the file content */
    if (filename) {
        FILE* fp;
        if (filename[0] == '-' && filename[1] == '\0') {
            fp = stdin;
        } else {
            if ((fp = fopen(filename,"r")) == NULL) {
                LOG_ERROR("Fatal error, can't open config file '%s'", filename);
                zfree(pconfig);
                exit(1);
            }
        }
        while (fgets(buf,MAX_STRING_LEN+1,fp) != NULL)
            config = sdscat(config,buf);
        if (fp != stdin) fclose(fp);
    }
    parse_string(pconfig, config);
    sdsfree(config);
    return pconfig;
}


void freeConfig(config_t* pconfig)
{
    if (!pconfig) {
        return;
    }
    zfree(pconfig->logfile_path);
    zfree(pconfig->script_dir);
    zfree(pconfig->pidfile_dir);
    zfree(pconfig->password);
    zfree(pconfig);
}

