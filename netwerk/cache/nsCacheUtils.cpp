





#include "nsCache.h"
#include "nsCacheUtils.h"
#include "nsThreadUtils.h"

using namespace mozilla;

class nsDestroyThreadEvent : public nsRunnable {
public:
  explicit nsDestroyThreadEvent(nsIThread *thread)
    : mThread(thread)
  {}
  NS_IMETHOD Run()
  {
    mThread->Shutdown();
    return NS_OK;
  }
private:
  nsCOMPtr<nsIThread> mThread;
};

nsShutdownThread::nsShutdownThread(nsIThread *aThread)
  : mLock("nsShutdownThread.mLock")
  , mCondVar(mLock, "nsShutdownThread.mCondVar")
  , mThread(aThread)
{
}

nsShutdownThread::~nsShutdownThread()
{
}

nsresult
nsShutdownThread::Shutdown(nsIThread *aThread)
{
  nsresult rv;
  nsRefPtr<nsDestroyThreadEvent> ev = new nsDestroyThreadEvent(aThread);
  rv = NS_DispatchToMainThread(ev);
  if (NS_FAILED(rv)) {
    NS_WARNING("Dispatching event in nsShutdownThread::Shutdown failed!");
  }
  return rv;
}

nsresult
nsShutdownThread::BlockingShutdown(nsIThread *aThread)
{
  nsresult rv;

  nsRefPtr<nsShutdownThread> st = new nsShutdownThread(aThread);
  nsCOMPtr<nsIThread> workerThread;

  rv = NS_NewNamedThread("thread shutdown", getter_AddRefs(workerThread));
  if (NS_FAILED(rv)) {
    NS_WARNING("Can't create nsShutDownThread worker thread!");
    return rv;
  }

  {
    MutexAutoLock lock(st->mLock);
    rv = workerThread->Dispatch(st, NS_DISPATCH_NORMAL);
    if (NS_FAILED(rv)) {
      NS_WARNING(
        "Dispatching event in nsShutdownThread::BlockingShutdown failed!");
    } else {
      st->mCondVar.Wait();
    }
  }

  return Shutdown(workerThread);
}

NS_IMETHODIMP
nsShutdownThread::Run()
{
  MutexAutoLock lock(mLock);
  mThread->Shutdown();
  mCondVar.Notify();
  return NS_OK;
}
