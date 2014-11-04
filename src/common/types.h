#ifndef _TYPES__H_
#define _TYPES__H_

#include "platform.h"
#include <stdint.h>


#define	UG_FS_DBL		"%lf"
#define	UG_FS_DBL_EXT(p)	"%." #p "lf"

#define UG_FS_SIZE_T		"%u"
typedef unsigned int ug_fs_size_t;			/* use this type only in calls to printf() for formatting size_t */

#define UG_PTR_SIZE		sizeof(void *)

#if defined(_WIN32)


#ifdef _UNICODE
#	define ug_stat(path, buf)		__ug_stat(path, buf)
#	define ug_open(pathname, flags)	__ug_open(pathname, flags | O_BINARY)
#else
#	define ug_stat(path, buf)		_stat64(path, buf)
#	define ug_open(pathname, flags)	open(pathname, flags | O_BINARY)
#endif

#ifdef UNICODE
#	include <strsafe.h>
#	define ug_wsnprintf StringCchPrintf
#	define ug_strlen wcslen
#	define ug_strchr wcschr
#	define ug_strstr wcsstr
#	define ug_fullpath _wfullpath
#else
#	define ug_wsnprintf UG_snprintf
#	define ug_strlen strlen
#	define ug_strchr strchr
#	define ug_strstr strstr
#	define ug_fullpath _fullpath
#endif

#ifndef __UINT64_C
#	define __UINT64_C(x)	x
#endif

#define ug_uint64_t unsigned __int64
#define UG_FS_UI64 "%I64u"
#define UG_FS_UO64 "%I64o"
#define UG_FS_UX64 "%I64x"

//#	define stat		_stat64
#ifndef snprintf
#define snprintf		_snprintf
#endif

#define alloca		_alloca

#ifndef uint32_t
#	define uint32_t	__int32
#endif

#ifndef PATH_SEPARATOR
#	define PATH_SEPARATOR	'\\'
#endif

typedef int                  pid_t;

#ifndef snprintf
#define snprintf		_snprintf
#endif

#else	/* _WINDOWS */

#	define ug_stat(path, buf)		stat(path, buf)
#	define ug_open(pathname, flags)	open(pathname, flags)

#	ifndef __UINT64_C
#		ifdef UINT64_C
#			define __UINT64_C(c) (UINT64_C(c))
#		else
#			define __UINT64_C(c) (c ## ULL)
#		endif
#	endif

#	define ug_uint64_t uint64_t
#	if __WORDSIZE == 64
#		define UG_FS_UI64 "%lu"
#		define UG_FS_UO64 "%lo"
#		define UG_FS_UX64 "%lx"
#		define UG_OFFSET 10000000000000000UL
#	else
#		ifdef HAVE_LONG_LONG_QU
#			define UG_FS_UI64 "%qu"
#			define UG_FS_UO64 "%qo"
#			define UG_FS_UX64 "%qx"
#		else
#			define UG_FS_UI64 "%llu"
#			define UG_FS_UO64 "%llo"
#			define UG_FS_UX64 "%llx"
#		endif
#		define UG_OFFSET 10000000000000000ULL
#	endif

#ifndef PATH_SEPARATOR
#	define PATH_SEPARATOR	'/'
#endif

#endif	/* _WINDOWS */


#define UG_STR2UINT64(uint, string) sscanf(string, UG_FS_UI64, &uint)
#define UG_CT2UINT64(uint,  string)  sscanf(string, UG_FS_UO64, &uint)
#define UG_HEX2UINT64(uint, string) sscanf(string, UG_FS_UX64, &uint)



#endif
