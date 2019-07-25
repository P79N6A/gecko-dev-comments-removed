




































#ifndef _NSPSMBACKGROUNDTHREAD_H_
#define _NSPSMBACKGROUNDTHREAD_H_

#include "nspr.h"
#include "nscore.h"
#include "mozilla/CondVar.h"
#include "mozilla/Mutex.h"

enum {
  ePSMThreadRunning = 0,
  ePSMThreadStopRequested = 1,
  ePSMThreadStopped = 2
};

class nsPSMBackgroundThread
{
protected:
  static void PR_CALLBACK nsThreadRunner(void *arg);
  virtual void Run(void) = 0;

  
  PRThread *mThreadHandle;

  
  
  
  
  mozilla::Mutex mMutex;

  
  mozilla::CondVar mCond;

  
  PRUint32 mExitState;

public:
  nsPSMBackgroundThread();
  virtual ~nsPSMBackgroundThread();

  nsresult startThread();
  void requestExit();

private:
  PRUint32 GetExitStateThreadSafe();
};


#endif
