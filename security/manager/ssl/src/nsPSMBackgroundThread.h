



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
  static void nsThreadRunner(void *arg);
  virtual void Run(void) = 0;

  
  PRThread *mThreadHandle;

  
  
  
  
  mozilla::Mutex mMutex;

  
  
  mozilla::CondVar mCond;

  bool exitRequested(::mozilla::MutexAutoLock const & proofOfLock) const;
  bool exitRequestedNoLock() const { return mExitState != ePSMThreadRunning; }
  nsresult postStoppedEventToMainThread(::mozilla::MutexAutoLock const & proofOfLock);

private:
  enum {
    ePSMThreadRunning = 0,
    ePSMThreadStopRequested = 1,
    ePSMThreadStopped = 2
  } mExitState;

  
  nsCString mName;

public:
  nsPSMBackgroundThread();
  virtual ~nsPSMBackgroundThread();

  nsresult startThread(const nsCSubstring & name);
  void requestExit();
};


#endif
