/*************************************************************************

  snaphu Windows platform compatibility header
  Provides POSIX API replacements for Windows builds.
  Single-tile mode only (no fork/wait/multi-process).

*************************************************************************/

#ifndef SNAPHU_PLATFORM_H
#define SNAPHU_PLATFORM_H

#ifdef _WIN32

#include <windows.h>
#include <process.h>
#include <io.h>
#include <direct.h>
#include <sys/stat.h>
#include <sys/types.h>

/* Replace POSIX headers */
/* unistd.h, sys/wait.h, sys/resource.h, sys/time.h are not available */

/* pid_t replacement */
#ifndef _PID_T_DEFINED
typedef int pid_t;
#define _PID_T_DEFINED
#endif

/* /dev/null replacement */
#undef NULLFILE
#define NULLFILE "NUL"

/* unlink replacement */
#define unlink _unlink

/* getcwd replacement */
#define getcwd _getcwd

/* stat/mkdir replacements */
#define stat _stat
#define mkdir(path, mode) _mkdir(path)
#ifndef S_IRUSR
#define S_IRUSR 0
#define S_IWUSR 0
#endif

/* sleep replacement (seconds -> milliseconds) */
static inline unsigned int sleep(unsigned int seconds){
  Sleep(seconds * 1000);
  return 0;
}

/* getpid replacement */
#define getpid _getpid

/* Signal handling - Windows only supports SIGINT, SIGTERM, SIGABRT, etc.
 * SIGHUP, SIGQUIT, SIGPIPE, SIGBUS, SIGALRM do not exist on Windows.
 * We define them as no-ops for CatchSignals(). */
#ifndef SIGHUP
#define SIGHUP   9999
#endif
#ifndef SIGQUIT
#define SIGQUIT  9998
#endif
#ifndef SIGPIPE
#define SIGPIPE  9997
#endif
#ifndef SIGBUS
#define SIGBUS   9996
#endif
#ifndef SIGALRM
#define SIGALRM  9995
#endif

/* CatchSignals: on Windows, only register real signals */
static inline int CatchSignals_Win32(void (*SigHandler)(int)){
  signal(SIGINT, SigHandler);
  signal(SIGTERM, SigHandler);
  signal(SIGABRT, SigHandler);
  signal(SIGFPE, SigHandler);
  signal(SIGSEGV, SigHandler);
  return 0;
}
#define CatchSignals CatchSignals_Win32

/* KillChildrenExit: no-op on Windows single-tile mode */
static inline void KillChildrenExit_Win32(int signum){
  fflush(NULL);
  fprintf(stderr, "Received signal %d\nExiting\n", signum);
  fflush(NULL);
  exit(1);
}

/* StartTimers: use Windows clock() as CPU time approximation */
static inline int StartTimers_Win32(time_t *tstart, double *cputimestart){
  *tstart = time(NULL);
  *cputimestart = (double)clock() / CLOCKS_PER_SEC;
  return 0;
}
#define StartTimers StartTimers_Win32

/* DisplayElapsedTime: Windows version */
static inline int DisplayElapsedTime_Win32(time_t tstart, double cputimestart){
  double cputime, walltime, seconds;
  long hours, minutes;
  time_t tstop;

  tstop = time(NULL);
  cputime = (double)clock() / CLOCKS_PER_SEC - cputimestart;
  if(cputime >= 0){
    hours = (long)floor(cputime / 3600);
    minutes = (long)floor((cputime - 3600 * hours) / 60);
    seconds = cputime - 3600 * hours - 60 * minutes;
    fprintf(stderr, "Elapsed processor time:   %ld:%02ld:%05.2f\n",
            hours, minutes, seconds);
  }
  if(tstart > 0 && tstop > 0){
    walltime = (double)(tstop - tstart);
    hours = (long)floor(walltime / 3600);
    minutes = (long)floor((walltime - 3600 * hours) / 60);
    seconds = walltime - 3600 * hours - 60 * minutes;
    fprintf(stderr, "Elapsed wall clock time:  %ld:%02ld:%02ld\n",
            hours, minutes, (long)seconds);
  }
  return 0;
}
#define DisplayElapsedTime DisplayElapsedTime_Win32

/* fork/wait: not used in single-tile mode, but provide stubs */
/* These are only referenced in the multi-tile code path in Unwrap(),
 * which we skip for v1 (ntilerow=1, ntilecol=1). */

/* ChildResetStreamPointers: stub for Windows (no child processes in single-tile) */
/* The original uses pid_t; we provide a compatible signature */

/* Signal handlers: SetDump, KillChildrenExit, SignalExit
 * On Windows, SIGHUP doesn't exist. We redefine to only handle SIGINT. */

#endif /* _WIN32 */

#endif /* SNAPHU_PLATFORM_H */
