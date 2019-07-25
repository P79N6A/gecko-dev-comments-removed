




































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

PRUint32 nsPSMBackgroundThread::GetExitStateThreadSafe()
{
  MutexAutoLock threadLock(mMutex);

  return mExitState;
}

void nsPSMBackgroundThread::requestExit()
{
  if (!mThreadHandle)
    return;

  {
    MutexAutoLock threadLock(mMutex);

    if (mExitState != ePSMThreadRunning)
      return;

    mExitState = ePSMThreadStopRequested;
    mCond.NotifyAll();
  }

  
  
  while (GetExitStateThreadSafe() < ePSMThreadStopped) {
    NS_ProcessNextEvent(nsnull, PR_FALSE);
    if (GetExitStateThreadSafe() < ePSMThreadStopped)
      PR_Sleep(PR_SecondsToInterval(5));
  }

  PR_JoinThread(mThreadHandle);
  mThreadHandle = nsnull;
}
