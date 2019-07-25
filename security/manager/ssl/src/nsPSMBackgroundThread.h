




































#ifndef _NSPSMBACKGROUNDTHREAD_H_
#define _NSPSMBACKGROUNDTHREAD_H_

#include "nspr.h"
#include "nscore.h"
#include "mozilla/CondVar.h"
#include "mozilla/Mutex.h"
#include "nsNSSComponent.h"

class nsPSMBackgroundThread
{
protected:
  static void PR_CALLBACK nsThreadRunner(void *arg);
  virtual void Run(void) = 0;

  
  PRThread *mThreadHandle;

  
  
  
  
  mozilla::Mutex mMutex;

  
  
  mozilla::CondVar mCond;

  PRBool exitRequested(::mozilla::MutexAutoLock const & proofOfLock) const;
  PRBool exitRequestedNoLock() const { return mExitState != ePSMThreadRunning; }
  nsresult postStoppedEventToMainThread(::mozilla::MutexAutoLock const & proofOfLock);

private:
  enum {
    ePSMThreadRunning = 0,
    ePSMThreadStopRequested = 1,
    ePSMThreadStopped = 2
  } mExitState;

public:
  nsPSMBackgroundThread();
  virtual ~nsPSMBackgroundThread();

  nsresult startThread();
  void requestExit();
};


#endif
