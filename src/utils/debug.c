#include "debug.h"
#include <stdio.h>
#include "common/defines.h"
#include "utils/logger.h"

#ifdef HAVE_BACKTRACE
#include <stdlib.h>
#include <execinfo.h>
#include <ucontext.h>
#include <fcntl.h>
#endif /* HAVE_BACKTRACE */

static void bugReportStart(void) {
    LOG_WARNING("\n\n=== BUG REPORT START: Cut & paste starting from here ===");
}

void _ugAssert(char *estr, char *file, int line) {
    bugReportStart();
    LOG_WARNING("=== ASSERTION FAILED ===");
    LOG_WARNING("==> %s:%d '%s' is not true",file,line,estr);
#ifdef HAVE_BACKTRACE
    LOG_WARNING("(forcing SIGSEGV in order to print the stack trace)");
#endif
    *((char *)-1) = 'x';
}

void _ugPanic(char *msg, char *file, int line) {
    bugReportStart();
    LOG_WARNING("------------------------------------------------");
    LOG_WARNING("!!! Software Failure. Press left mouse button to continue");
    LOG_WARNING("Guru Meditation: %s #%s:%d",msg,file,line);
#ifdef HAVE_BACKTRACE
    LOG_WARNING("(forcing SIGSEGV in order to print the stack trace)");
#endif
    LOG_WARNING("------------------------------------------------");
    *((char *)-1) = 'x';
}

#ifdef HAVE_BACKTRACE

static void *getMcontextEip(ucontext_t *uc) {
#if defined(__APPLE__) && !defined(MAC_OS_X_VERSION_10_6)
    /* OSX < 10.6 */
#if defined(__x86_64__)
    return (void *) uc->uc_mcontext->__ss.__rip;
#elif defined(__i386__)
    return (void *) uc->uc_mcontext->__ss.__eip;
#else
    return (void *) uc->uc_mcontext->__ss.__srr0;
#endif
#elif defined(__APPLE__) && defined(MAC_OS_X_VERSION_10_6)
    /* OSX >= 10.6 */
#if defined(_STRUCT_X86_THREAD_STATE64) && !defined(__i386__)
    return (void *) uc->uc_mcontext->__ss.__rip;
#else
    return (void *) uc->uc_mcontext->__ss.__eip;
#endif
#elif defined(__linux__)
    /* Linux */
#if defined(__i386__)
    return (void *) uc->uc_mcontext.gregs[14]; /* Linux 32 */
#elif defined(__X86_64__) || defined(__x86_64__)
    return (void *) uc->uc_mcontext.gregs[16]; /* Linux 64 */
#elif defined(__ia64__) /* Linux IA64 */
    return (void *) uc->uc_mcontext.sc_ip;
#endif
#else
    return NULL;
#endif
}

/* Logs the stack trace using the backtrace() call. This function is designed
 * to be called from signal handlers safely. */
static void stacktrace(ucontext_t *uc) {
    void *trace[100];
    char **strings;
    int trace_size = 0, idx;

    /* Generate the stack trace */
    trace_size = backtrace(trace, 100);

    /* overwrite sigaction with caller's address */
    if (getMcontextEip(uc) != NULL)
        trace[1] = getMcontextEip(uc);

    /* Write symbols to log file */
    strings = backtrace_symbols(trace, trace_size);

    for (idx=0; idx<trace_size; ++idx) {
        printf("%s", strings[idx]);
    }

    free(strings);
}

void sigsegvHandler(int sig, siginfo_t *info, void *secret) {
    ucontext_t *uc = (ucontext_t *) secret;
    struct sigaction act;
    printf("%s %s crashed by signal: %d", APPNAME, VERSION, sig);

    /* Log the stack trace */
    printf("--- STACK TRACE START ---");
    stacktrace(uc);
    printf("--- STACK TRACE END ---");
    /* Make sure we exit with the right signal at the end. So for instance
     * the core will be dumped if enabled. */
    sigemptyset (&act.sa_mask);
    act.sa_flags = SA_NODEFER | SA_ONSTACK | SA_RESETHAND;
    act.sa_handler = SIG_DFL;
    sigaction (sig, &act, NULL);
    kill(getpid(),sig);
}

#endif /* HAVE_BACKTRACE */
