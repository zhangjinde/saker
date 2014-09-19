#ifndef _CONFIG__H_
#define _CONFIG__H_
/**
* config 
*/

#define LOGFILE_PATH         "logfile"
#define LOGFILE_LEVEL        "loglevel"
#define SCRIPT_DIR           "script-dir"
#define PIDFILE_DIR          "pidfile-dir"
#define WORK_INTERVAL        "interval"
#define BIND                 "bind"
#define LOCAL_PORT           "port"
#define NET_TIMEOUT          "timeout"
#define TOP_MODE             "top-mode"
#define MAXCLIENTS           "maxclients"
#define PASSWORD             "password"


typedef struct config {
    int      work_interval;
    char*    logfile_path;
    int      logfile_level;
    int      net_keeplive;
    int      net_timeout;
    char*    script_dir;
    char*    pidfile_dir;
    char*    password;
    char*    bind;
    int      port;
    int      top_mode;
    int      maxclients;
} config_t;


config_t*  createConfig(const char* file );


void  freeConfig(config_t* pconfig);




#endif
