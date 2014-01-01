#ifndef _COMMANDS__H_
#define _COMMANDS__H_

#include "utils/udict.h"

struct ugClient;
typedef void (*ugCommandProc)(struct ugClient *c);

typedef struct ugCommand {
    char* name;
    ugCommandProc proc;
    int  params;    /* The actual flags. -1 means transformable , otherwise that means the real params */
    long long microseconds, calls;
} ugCommand;

dict* createCommands(void);

void destroyCommands(dict* );

ugCommand* lookupCommand(char* key);


#endif
