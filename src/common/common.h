#ifndef _COMMON__H_
#define _COMMON__H_

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "types.h"
#include "event/zmalloc.h"
#include "common/winfixes.h"



#ifndef MAX
#	define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#	define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif



#define FILECLOSE(file)	\
				\
do				\
{				\
	if (file)		\
	{			\
		fclose(file);	\
		file = NULL;	\
	}			\
}				\
while (0)

#define MAX_STRING_LEN (1024)


#ifndef S_ISREG
#	define S_ISREG(x) (((x) & S_IFMT) == S_IFREG)
#endif

#ifndef S_ISDIR
#	define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#endif

#define UG_STR2UINT64(uint, string) sscanf(string, UG_FS_UI64, &uint)
#define UG_OCT2UINT64(uint, string) sscanf(string, UG_FS_UO64, &uint)
#define UG_HEX2UINT64(uint, string) sscanf(string, UG_FS_UX64, &uint)

#define UG_CONST_STRING(str) ""str

#define UG_NOTUSED(V) ((void) V)


#ifdef OS_UNIX
#define ENDFLAG             "\n"
#else
#define ENDFLAG             "\r\n"
#endif

#endif
