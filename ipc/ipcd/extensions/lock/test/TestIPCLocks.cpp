











































#include <stdlib.h>
#include <stdio.h>
#include "ipcILockService.h"
#include "ipcLockCID.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "nsCOMPtr.h"
#include "nsXPCOM.h"
#include "prproces.h"
#include "prprf.h"

#if defined(XP_UNIX) || defined(XP_OS2) || defined(XP_BEOS)
#include <unistd.h>
static unsigned GetPID()
{
  return (unsigned) getpid();
}
#elif defined(XP_WIN)
#include <windows.h>
static unsigned GetPID()
{
  return (unsigned) GetCurrentProcessId();  
}
#else
static unsigned int GetPID()
{
  return 0; 
}
#endif

static void LOG(const char *fmt, ... )
{
  va_list ap;
  va_start(ap, fmt);
  PRUint32 nb = 0;
  char buf[512];

  nb = PR_snprintf(buf, sizeof(buf), "[%u:%p] ", GetPID(), PR_GetCurrentThread());

  PR_vsnprintf(buf + nb, sizeof(buf) - nb, fmt, ap);
  buf[sizeof(buf) - 1] = '\0';

  fwrite(buf, strlen(buf), 1, stdout);
  fflush(stdout);

  va_end(ap);
}

static void RandomSleep(PRUint32 fromMS, PRUint32 toMS)
{
  PRUint32 ms = fromMS + (PRUint32) ((toMS - fromMS) * ((double) rand() / RAND_MAX));
  
  PR_Sleep(PR_MillisecondsToInterval(ms));
}

static ipcILockService *gLockService;

PR_STATIC_CALLBACK(void) TestThread(void *arg)
{
  const char *lockName = (const char *) arg;

  LOG("entering TestThread [lock=%s]\n", lockName);

  nsresult rv;

  RandomSleep(1000, 1100);

  

  rv = gLockService->AcquireLock(lockName, PR_TRUE);
  if (NS_SUCCEEDED(rv))
  {
    
    RandomSleep(500, 1000);
    
    rv = gLockService->ReleaseLock(lockName);
    if (NS_FAILED(rv))
    {
      LOG("failed to release lock [rv=%x]\n", rv);
      NS_ERROR("failed to release lock");
    }
  }
  else
  {
    LOG("failed to acquire lock [rv=%x]\n", rv);
    NS_NOTREACHED("failed to acquire lock");
  }

  LOG("exiting TestThread [lock=%s rv=%x]\n", lockName, rv);
}

static const char *kLockNames[] = {
  "foopy",
  "test",
  "1",
  "xyz",
  "moz4ever",
  nsnull
};

static nsresult DoTest()
{
  nsCOMPtr<ipcILockService> lockService =
      do_GetService(IPC_LOCKSERVICE_CONTRACTID);

  gLockService = lockService;

  PRThread *threads[10] = {0};
  int i = 0;

  for (const char **lockName = kLockNames; *lockName; ++lockName, ++i)
  {
    threads[i] = PR_CreateThread(PR_USER_THREAD,
                                 TestThread,
                                 (void *) *lockName,
                                 PR_PRIORITY_NORMAL,
                                 PR_GLOBAL_THREAD,
                                 PR_JOINABLE_THREAD,
                                 0);
  }

  for (i=0; threads[i]; ++i)
  {
    PR_JoinThread(threads[i]);
    threads[i] = nsnull;
  }

  gLockService = nsnull;

  LOG("joined with all threads; exiting DoTest\n");
  return NS_OK;
}

int main(int argc, char **argv)
{
  LOG("entering main\n");

  int numProcs = 10;

  
  if (argc > 1)
  {
    if (strcmp(argv[1], "-child") == 0)
    {
      RandomSleep(1000, 1000);
      LOG("running child test\n");
      NS_InitXPCOM2(nsnull, nsnull, nsnull);
      DoTest();
      NS_ShutdownXPCOM(nsnull);
      return 0;
    }
    else if (argv[1][0] == '-')
    {
      
      numProcs = atoi(argv[1] + 1);
      if (numProcs == 0)
      {
        printf("### usage: TestIPCLocks [-N]\n"
               "where, N is the number of test processes to spawn.\n");
        return -1;
      }
    }
  }

  LOG("sleeping for 1 second\n");
  PR_Sleep(PR_SecondsToInterval(1));

  PRProcess **procs = (PRProcess **) malloc(sizeof(PRProcess*) * numProcs);
  int i;

  
  for (i=0; i<numProcs; ++i)
  {
    char *const argv[] = {"./TestIPCLocks", "-child", nsnull};
    LOG("spawning child test\n");
    procs[i] = PR_CreateProcess("./TestIPCLocks", argv, nsnull, nsnull);
  }

  PRInt32 exitCode;
  for (i=0; i<numProcs; ++i)
    PR_WaitProcess(procs[i], &exitCode);
  
  return 0;
}
