











































#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "prthread.h"
#include "plstr.h"
#include "prenv.h"
#include "nsDebug.h"

#if defined(LINUX)
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdlib.h> 
#endif

#if defined(SOLARIS)
#include <sys/resource.h>
#endif

#ifdef XP_BEOS
#include <be/app/Application.h>
#include <string.h>
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIAppStartup.h"
#include "nsXPFEComponentsCID.h"
#endif

#ifdef MOZ_WIDGET_PHOTON
#include <photon/PhProto.h>
#endif

static char _progname[1024] = "huh?";
static unsigned int _gdb_sleep_duration = 300;

#if defined(LINUX) && defined(DEBUG) && \
      (defined(__i386) || defined(__x86_64) || defined(PPC))
#define CRAWL_STACK_ON_SIGSEGV
#endif

#ifdef MOZ_WIDGET_PHOTON
void abnormal_exit_handler(int signum)
{
  
  PgShmemCleanup();

#if defined(DEBUG)
  if (    (signum == SIGSEGV)
       || (signum == SIGILL)
       || (signum == SIGABRT)
       || (signum == SIGFPE)
     )
  {
    printf("prog = %s\npid = %d\nsignal = %s\n", 
           _progname, getpid(), strsignal(signum));

    printf("Sleeping for %d seconds.\n",_gdb_sleep_duration);
    printf("Type 'gdb %s %d' to attach your debugger to this thread.\n",
           _progname, getpid());

    sleep(_gdb_sleep_duration);

    printf("Done sleeping...\n");
  }
#endif

  _exit(1);
}
#elif defined(CRAWL_STACK_ON_SIGSEGV)

#include <unistd.h>
#include "nsISupportsUtils.h"
#include "nsStackWalk.h"

extern "C" {

static void PrintStackFrame(void *aPC, void *aClosure)
{
  char buf[1024];
  nsCodeAddressDetails details;

  NS_DescribeCodeAddress(aPC, &details);
  NS_FormatCodeAddressDetails(aPC, &details, buf, sizeof(buf));
  fputs(buf, stdout);
}

}

void
ah_crap_handler(int signum)
{
  printf("\nProgram %s (pid = %d) received signal %d.\n",
         _progname,
         getpid(),
         signum);

  printf("Stack:\n");
  NS_StackWalk(PrintStackFrame, 2, nsnull);

  printf("Sleeping for %d seconds.\n",_gdb_sleep_duration);
  printf("Type 'gdb %s %d' to attach your debugger to this thread.\n",
         _progname,
         getpid());

  sleep(_gdb_sleep_duration);

  printf("Done sleeping...\n");

  _exit(signum);
}
#endif 

#ifdef XP_BEOS
void beos_signal_handler(int signum) {
#ifdef DEBUG
	fprintf(stderr, "beos_signal_handler: %d\n", signum);
#endif
	nsresult rv;
	nsCOMPtr<nsIAppStartup> appStartup(do_GetService(NS_APPSTARTUP_CONTRACTID, &rv));
	if (NS_FAILED(rv)) {
		
#ifdef DEBUG
		fprintf(stderr, "beos_signal_handler: appShell->do_GetService() failed\n");
#endif
		exit(13);
	}

	
	appStartup->Quit(nsIAppStartup::eAttemptQuit);
}
#endif

#ifdef MOZ_WIDGET_GTK2

#include <glib.h>
#endif

#if defined(MOZ_WIDGET_GTK2) && (GLIB_MAJOR_VERSION > 2 || (GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION >= 6))

static GLogFunc orig_log_func = NULL;

extern "C" {
static void
my_glib_log_func(const gchar *log_domain, GLogLevelFlags log_level,
                 const gchar *message, gpointer user_data);
}

 void
my_glib_log_func(const gchar *log_domain, GLogLevelFlags log_level,
                 const gchar *message, gpointer user_data)
{
  if (log_level & (G_LOG_LEVEL_ERROR | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION)) {
    NS_DebugBreak(NS_DEBUG_ASSERTION, message, "glib assertion", __FILE__, __LINE__);
  } else if (log_level & (G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING)) {
    NS_DebugBreak(NS_DEBUG_WARNING, message, "glib warning", __FILE__, __LINE__);
  }

  orig_log_func(log_domain, log_level, message, NULL);
}

#endif

void InstallUnixSignalHandlers(const char *ProgramName)
{
  PL_strncpy(_progname,ProgramName, (sizeof(_progname)-1) );

  const char *gdbSleep = PR_GetEnv("MOZ_GDB_SLEEP");
  if (gdbSleep && *gdbSleep)
  {
    unsigned int s;
    if (1 == sscanf(gdbSleep, "%u", &s)) {
      _gdb_sleep_duration = s;
    }
  }

#if defined(MOZ_WIDGET_PHOTON)
 
  signal(SIGTERM, abnormal_exit_handler);
  signal(SIGQUIT, abnormal_exit_handler);
  signal(SIGINT,  abnormal_exit_handler);
  signal(SIGHUP,  abnormal_exit_handler);
  signal(SIGSEGV, abnormal_exit_handler);
  signal(SIGILL,  abnormal_exit_handler);
  signal(SIGABRT, abnormal_exit_handler);
  signal(SIGFPE,  abnormal_exit_handler);

#elif defined(CRAWL_STACK_ON_SIGSEGV)
  signal(SIGSEGV, ah_crap_handler);
  signal(SIGILL, ah_crap_handler);
  signal(SIGABRT, ah_crap_handler);
  signal(SIGFPE, ah_crap_handler);
#endif 

#if defined(DEBUG) && defined(LINUX)
  const char *memLimit = PR_GetEnv("MOZ_MEM_LIMIT");
  if (memLimit && *memLimit)
  {
    long m = atoi(memLimit);
    m *= (1024*1024);
    struct rlimit r;
    r.rlim_cur = m;
    r.rlim_max = m;
    setrlimit(RLIMIT_AS, &r);
  }
#endif

#if defined(SOLARIS)
#define NOFILES 512

    
    {
	struct rlimit rl;
	
	if (getrlimit(RLIMIT_NOFILE, &rl) == 0)

	    if (rl.rlim_cur < NOFILES) {
		rl.rlim_cur = NOFILES;

		if (setrlimit(RLIMIT_NOFILE, &rl) < 0) {
		    perror("setrlimit(RLIMIT_NOFILE)");
		    fprintf(stderr, "Cannot exceed hard limit for open files");
		}
#if defined(DEBUG)
	    	if (getrlimit(RLIMIT_NOFILE, &rl) == 0)
		    printf("File descriptors set to %d\n", rl.rlim_cur);
#endif 
	    }
    }
#endif 

#ifdef XP_BEOS
	signal(SIGTERM, beos_signal_handler);
#endif

#if defined(MOZ_WIDGET_GTK2) && (GLIB_MAJOR_VERSION > 2 || (GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION >= 6))
  const char *assertString = PR_GetEnv("XPCOM_DEBUG_BREAK");
  if (assertString &&
      (!strcmp(assertString, "suspend") ||
       !strcmp(assertString, "stack") ||
       !strcmp(assertString, "abort") ||
       !strcmp(assertString, "trap") ||
       !strcmp(assertString, "break"))) {
    
    orig_log_func = g_log_set_default_handler(my_glib_log_func, NULL);
  }
#endif
}
