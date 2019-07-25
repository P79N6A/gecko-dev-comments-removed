




































#include "nsPSMBackgroundThread.h"
#include "nsThreadUtils.h"

using namespace mozilla;

void PR_CALLBACK nsPSMBackgroundThread::nsThreadRunner(void *arg)
{
  nsPSMBackgroundThread *self = static_cast<nsPSMBackgroundThread *>(arg);
  self->Run();
}

nsPSMBackgroundThread::nsPSMBackgroundThread()
: mThreadHandle(nsnull),
  mMutex("nsPSMBackgroundThread.mMutex"),
  mCond(mMutex, "nsPSMBackgroundThread.mCond"),
  mExitState(ePSMThreadRunning)
{
}

nsresult nsPSMBackgroundThread::startThread()
{
  mThreadHandle = PR_CreateThread(PR_USER_THREAD, nsThreadRunner, static_cast<void*>(this), 
    PR_PRIORITY_NORMAL, PR_LOCAL_THREAD, PR_JOINABLE_THREAD, 0);

  NS_ASSERTION(mThreadHandle, "Could not create nsPSMBackgroundThread\n");
  
  if (!mThreadHandle)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

nsPSMBackgroundThread::~nsPSMBackgroundThread()
{
}

PRBool
nsPSMBackgroundThread::exitRequested(const MutexAutoLock & ) const
{
  return exitRequestedNoLock();
}

nsresult
nsPSMBackgroundThread::postStoppedEventToMainThread(
    MutexAutoLock const & )
{
  NS_ASSERTION(PR_GetCurrentThread() == mThreadHandle,
               "Background thread stopped from another thread");

  mExitState = ePSMThreadStopped;
  
  return NS_DispatchToMainThread(new nsRunnable());
}

void nsPSMBackgroundThread::requestExit()
{
  NS_ASSERTION(NS_IsMainThread(),
               "nsPSMBackgroundThread::requestExit called off main thread.");

  if (!mThreadHandle)
    return;

  {
    MutexAutoLock threadLock(mMutex);
    if (mExitState < ePSMThreadStopRequested) {
      mExitState = ePSMThreadStopRequested;
      mCond.NotifyAll();
    }
  }
  
  nsCOMPtr<nsIThread> mainThread = do_GetCurrentThread();
  for (;;) {
    {
      MutexAutoLock threadLock(mMutex);
      if (mExitState == ePSMThreadStopped)
        break;
    }
    NS_ProcessPendingEvents(mainThread, PR_MillisecondsToInterval(50));
  }

  PR_JoinThread(mThreadHandle);
  mThreadHandle = nsnull;
}
