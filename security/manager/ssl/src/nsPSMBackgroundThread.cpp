




































#include "nsPSMBackgroundThread.h"

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
  mExitRequested(PR_FALSE)
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

void nsPSMBackgroundThread::requestExit()
{
  if (!mThreadHandle)
    return;

  {
    MutexAutoLock threadLock(mMutex);

    if (mExitRequested)
      return;

    mExitRequested = PR_TRUE;
    mCond.NotifyAll();
  }

  PR_JoinThread(mThreadHandle);
  mThreadHandle = nsnull;
}
