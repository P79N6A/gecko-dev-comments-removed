




































#include "nsPSMBackgroundThread.h"
#include "nsAutoLock.h"

void PR_CALLBACK nsPSMBackgroundThread::nsThreadRunner(void *arg)
{
  nsPSMBackgroundThread *self = NS_STATIC_CAST(nsPSMBackgroundThread *, arg);
  self->Run();
}

nsPSMBackgroundThread::nsPSMBackgroundThread()
: mThreadHandle(nsnull),
  mMutex(nsnull),
  mCond(nsnull),
  mExitRequested(PR_FALSE)
{
  mMutex = PR_NewLock();
  mCond = PR_NewCondVar(mMutex);
}

nsresult nsPSMBackgroundThread::startThread()
{
  if (!mMutex || !mCond)
    return NS_ERROR_OUT_OF_MEMORY;

  mThreadHandle = PR_CreateThread(PR_USER_THREAD, nsThreadRunner, NS_STATIC_CAST(void*, this), 
    PR_PRIORITY_NORMAL, PR_LOCAL_THREAD, PR_JOINABLE_THREAD, 0);

  NS_ASSERTION(mThreadHandle, "Could not create nsPSMBackgroundThread\n");
  
  if (!mThreadHandle)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

nsPSMBackgroundThread::~nsPSMBackgroundThread()
{
  if (mCond)
    PR_DestroyCondVar(mCond);

  if (mMutex)
    PR_DestroyLock(mMutex);
}

void nsPSMBackgroundThread::requestExit()
{
  if (!mThreadHandle)
    return;

  {
    nsAutoLock threadLock(mMutex);

    if (mExitRequested)
      return;

    mExitRequested = PR_TRUE;
    PR_NotifyAllCondVar(mCond);
  }

  PR_JoinThread(mThreadHandle);
  mThreadHandle = nsnull;
}
