




































#include "nsPSMBackgroundThread.h"
#include "nsAutoLock.h"

void PR_CALLBACK nsPSMBackgroundThread::nsThreadRunner(void *arg)
{
  nsPSMBackgroundThread *self = static_cast<nsPSMBackgroundThread *>(arg);
  self->Run();
}

nsPSMBackgroundThread::nsPSMBackgroundThread()
: mThreadHandle(nsnull),
  mMutex(nsnull),
  mCond(nsnull),
  mExitRequested(PR_FALSE)
{
  mMutex = nsAutoLock::NewLock("nsPSMBackgroundThread::mMutex");
  mCond = PR_NewCondVar(mMutex);
}

nsresult nsPSMBackgroundThread::startThread()
{
  if (!mMutex || !mCond)
    return NS_ERROR_OUT_OF_MEMORY;

  mThreadHandle = PR_CreateThread(PR_USER_THREAD, nsThreadRunner, static_cast<void*>(this), 
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
    nsAutoLock::DestroyLock(mMutex);
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
