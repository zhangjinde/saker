#ifndef _PLATFORM__H_
#define _PLATFORM__H_


#if defined(__FreeBSD__)
#define OS_UNIX     (1)
#define OS_BSD      (1)
#define OS_TYPE     "UNIX"
#define OS_STRING   "FREE_BSD"
#elif defined(_AIX) || defined(__TOS_AIX__)
#define OS_UNIX     (1)
#define OS_AIX      (1)
#define OS_TYPE     "UNIX"
#define OS_STRING   "AIX"
#elif defined(hpux) || defined(_hpux)
#define OS_UNIX     (1)
#define OS_HP       (1)
#define OS_TYPE     "UNIX"
#define OS_STRING   "HPUX"
#elif defined(__digital__) || defined(__osf__)
#define OS_UNIX    (1)
#define OS_TYPE     "UNIX"
#define OS_STRING   "TRU64"
#elif defined(linux) || defined(__linux) || defined(__linux__) || defined(__TOS_LINUX__)
#define OS_UNIX     (1)
#define OS_TYPE     "UNIX"
#define OS_STRING   "LINUX"
#elif defined(__APPLE__) || defined(__TOS_MACOS__)
#define OS_UNIX     (1)
#define OS_BSD      (1)
#define OS_TYPE     "UNIX"
#define OS_STRING   "MAC_OS_X"
#elif defined(__NetBSD__)
#define OS_UNIX     (1)
#define OS_BSD      (1)
#define OS_TYPE     "UNIX"
#define OS_STRING   "NET_BSD"
#elif defined(__OpenBSD__)
#define OS_UNIX     (1)
#define OS_BSD      (1)
#define OS_TYPE     "UNIX"
#define OS_STRING   "OPEN_BSD"
#elif defined(sgi) || defined(__sgi)
#define OS_UNIX     (1)
#define OS_TYPE     "UNIX"
#define OS_STRING   "OPEN_IRIX"
#elif defined(sun) || defined(__sun)
#define OS_UNIX     (1)
#define OS_TYPE     "UNIX"
#define OS_STRING   "SOLARIS"
#elif defined(__QNX__)
#define OS_UNIX     (1)
#define OS_TYPE     "UNIX"
#define OS_STRING   "QNX"
#elif defined(__CYGWIN__)
#define OS_UNIX     (1)
#define OS_TYPE     "UNIX"
#define OS_STRING   "CYGWIN"
#elif defined(unix) || defined(__unix) || defined(__unix__)
#define OS_UNIX     (1)
#define OS_TYPE     "UNIX"
#define OS_STRING   "UNKNOWN_UNIX"
#elif defined(_WIN32_WCE)
#error "can't support this platform"
//#define _WINDOWS 1
//#define _WINDOWS_CE
#elif defined(_WIN32) || defined(_WIN64)
#define OS_WIN		(1)
#define OS_TYPE     "WIN"
#define OS_STRING   "WINDOWS_NT"
#elif defined(__VMS)
//#define _FAMILY_VMS 1
//#define _OS_VMS
#error "can't support this platform"
#else
#error "can't support this platform"
#endif



/* Pull in platform-specific headers required by built-in functions */
#if OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <process.h>
#include <Iphlpapi.h>
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Advapi32.lib")
#else
#include <unistd.h>
#endif

/* Test for backtrace() */
#if defined(__APPLE__) || defined(__linux__)
#define HAVE_BACKTRACE 1
#endif


/* A success return code */
#define UGOK   (0)  
#define	UGERR	  (-1)

#endif
