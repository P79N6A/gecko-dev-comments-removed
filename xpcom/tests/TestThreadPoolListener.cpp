




































#include "TestHarness.h"

#include "nsIProxyObjectManager.h"
#include "nsIThread.h"
#include "nsIThreadPool.h"

#include "nsThreadUtils.h"
#include "nsXPCOMCIDInternal.h"
#include "pratom.h"
#include "prinrval.h"
#include "prmon.h"
#include "prthread.h"

#include "mozilla/Monitor.h"
using namespace mozilla;

#define NUMBER_OF_THREADS 4


#define IDLE_THREAD_TIMEOUT 3600000

static nsIThread** gCreatedThreadList = nsnull;
static nsIThread** gShutDownThreadList = nsnull;

static Monitor* gMonitor = nsnull;

static PRBool gAllRunnablesPosted = PR_FALSE;
static PRBool gAllThreadsCreated = PR_FALSE;
static PRBool gAllThreadsShutDown = PR_FALSE;

#ifdef DEBUG
#define TEST_ASSERTION(_test, _msg) \
    NS_ASSERTION(_test, _msg);
#else
#define TEST_ASSERTION(_test, _msg) \
  PR_BEGIN_MACRO \
    if (!(_test)) { \
      NS_DebugBreak(NS_DEBUG_ABORT, _msg, #_test, __FILE__, __LINE__); \
    } \
  PR_END_MACRO
#endif

class Listener : public nsIThreadPoolListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITHREADPOOLLISTENER
};

NS_IMPL_THREADSAFE_ISUPPORTS1(Listener, nsIThreadPoolListener)

NS_IMETHODIMP
Listener::OnThreadCreated()
{
  nsCOMPtr<nsIThread> current(do_GetCurrentThread());
  TEST_ASSERTION(current, "Couldn't get current thread!");

  MonitorAutoEnter mon(*gMonitor);

  while (!gAllRunnablesPosted) {
    mon.Wait();
  }

  for (PRUint32 i = 0; i < NUMBER_OF_THREADS; i++) {
    nsIThread* thread = gCreatedThreadList[i];
    TEST_ASSERTION(thread != current, "Saw the same thread twice!");

    if (!thread) {
      gCreatedThreadList[i] = current;
      if (i == (NUMBER_OF_THREADS - 1)) {
        gAllThreadsCreated = PR_TRUE;
        mon.NotifyAll();
      }
      return NS_OK;
    }
  }

  TEST_ASSERTION(PR_FALSE, "Too many threads!");
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
Listener::OnThreadShuttingDown()
{
  nsCOMPtr<nsIThread> current(do_GetCurrentThread());
  TEST_ASSERTION(current, "Couldn't get current thread!");

  MonitorAutoEnter mon(*gMonitor);

  for (PRUint32 i = 0; i < NUMBER_OF_THREADS; i++) {
    nsIThread* thread = gShutDownThreadList[i];
    TEST_ASSERTION(thread != current, "Saw the same thread twice!");

    if (!thread) {
      gShutDownThreadList[i] = current;
      if (i == (NUMBER_OF_THREADS - 1)) {
        gAllThreadsShutDown = PR_TRUE;
        mon.NotifyAll();
      }
      return NS_OK;
    }
  }

  TEST_ASSERTION(PR_FALSE, "Too many threads!");
  return NS_ERROR_FAILURE;
}

class AutoCreateAndDestroyMonitor
{
public:
  AutoCreateAndDestroyMonitor(Monitor** aMonitorPtr)
  : mMonitorPtr(aMonitorPtr) {
    *aMonitorPtr = new Monitor("TestThreadPoolListener::AutoMon");
    TEST_ASSERTION(*aMonitorPtr, "Out of memory!");
  }

  ~AutoCreateAndDestroyMonitor() {
    if (*mMonitorPtr) {
      delete *mMonitorPtr;
      *mMonitorPtr = nsnull;
    }
  }

private:
  Monitor** mMonitorPtr;
};

int main(int argc, char** argv)
{
  ScopedXPCOM xpcom("ThreadPoolListener");
  NS_ENSURE_FALSE(xpcom.failed(), 1);

  nsIThread* createdThreadList[NUMBER_OF_THREADS] = { nsnull };
  gCreatedThreadList = createdThreadList;

  nsIThread* shutDownThreadList[NUMBER_OF_THREADS] = { nsnull };
  gShutDownThreadList = shutDownThreadList;

  AutoCreateAndDestroyMonitor newMon(&gMonitor);
  NS_ENSURE_TRUE(gMonitor, 1);

  nsresult rv;

  
  
  
  nsCOMPtr<nsIProxyObjectManager> proxyObjMgr =
    do_GetService(NS_XPCOMPROXY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIThreadPool> pool =
    do_CreateInstance(NS_THREADPOOL_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, 1);

  rv = pool->SetThreadLimit(NUMBER_OF_THREADS);
  NS_ENSURE_SUCCESS(rv, 1);

  rv = pool->SetIdleThreadLimit(NUMBER_OF_THREADS);
  NS_ENSURE_SUCCESS(rv, 1);

  rv = pool->SetIdleThreadTimeout(IDLE_THREAD_TIMEOUT);
  NS_ENSURE_SUCCESS(rv, 1);

  nsCOMPtr<nsIThreadPoolListener> listener = new Listener();
  NS_ENSURE_TRUE(listener, 1);

  rv = pool->SetListener(listener);
  NS_ENSURE_SUCCESS(rv, 1);

  {
    MonitorAutoEnter mon(*gMonitor);

    for (PRUint32 i = 0; i < NUMBER_OF_THREADS; i++) {
      nsCOMPtr<nsIRunnable> runnable = new nsRunnable();
      NS_ENSURE_TRUE(runnable, 1);

      rv = pool->Dispatch(runnable, NS_DISPATCH_NORMAL);
      NS_ENSURE_SUCCESS(rv, 1);
    }

    gAllRunnablesPosted = PR_TRUE;
    mon.NotifyAll();
  }

  {
    MonitorAutoEnter mon(*gMonitor);
    while (!gAllThreadsCreated) {
      mon.Wait();
    }
  }

  rv = pool->Shutdown();
  NS_ENSURE_SUCCESS(rv, 1);

  {
    MonitorAutoEnter mon(*gMonitor);
    while (!gAllThreadsShutDown) {
      mon.Wait();
    }
  }

  for (PRUint32 i = 0; i < NUMBER_OF_THREADS; i++) {
    nsIThread* created = gCreatedThreadList[i];
    NS_ENSURE_TRUE(created, 1);

    PRBool match = PR_FALSE;
    for (PRUint32 j = 0; j < NUMBER_OF_THREADS; j++) {
      nsIThread* destroyed = gShutDownThreadList[j];
      NS_ENSURE_TRUE(destroyed, 1);

      if (destroyed == created) {
        match = PR_TRUE;
        break;
      }
    }

    NS_ENSURE_TRUE(match, 1);
  }

  return 0;
}
