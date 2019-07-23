





































#include <stdio.h>
#include <stdlib.h>
#include "nsXPCOM.h"
#include "nsXPCOMCIDInternal.h"
#include "nsIThreadPool.h"
#include "nsComponentManagerUtils.h"
#include "nsCOMPtr.h"
#include "nsIRunnable.h"

class Task : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  Task(int i) : mIndex(i) {}

  NS_IMETHOD Run()
  {
    printf("###(%d) running from thread: %p\n", mIndex, (void *) PR_GetCurrentThread());
    int r = (int) ((float) rand() * 200 / RAND_MAX);
    PR_Sleep(PR_MillisecondsToInterval(r));
    printf("###(%d) exiting from thread: %p\n", mIndex, (void *) PR_GetCurrentThread());
    return NS_OK;
  }

private:
  int mIndex;
};
NS_IMPL_THREADSAFE_ISUPPORTS1(Task, nsIRunnable)

static nsresult
RunTests()
{
  nsCOMPtr<nsIThreadPool> pool = do_CreateInstance(NS_THREADPOOL_CONTRACTID);
  NS_ENSURE_STATE(pool);

  for (int i = 0; i < 100; ++i) {
    nsCOMPtr<nsIRunnable> task = new Task(i);
    NS_ENSURE_TRUE(task, NS_ERROR_OUT_OF_MEMORY);

    pool->Dispatch(task, NS_DISPATCH_NORMAL);
  }

  pool->Shutdown();
  return NS_OK;
}

int
main(int argc, char **argv)
{
  if (NS_FAILED(NS_InitXPCOM2(nsnull, nsnull, nsnull)))
    return -1;
  RunTests();
  NS_ShutdownXPCOM(nsnull);
  return 0;
}
