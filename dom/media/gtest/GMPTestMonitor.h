





#include "nsThreadUtils.h"

#ifndef __GMPTestMonitor_h__
#define __GMPTestMonitor_h__

class GMPTestMonitor
{
public:
  GMPTestMonitor()
    : mFinished(false)
  {
  }

  void AwaitFinished()
  {
    MOZ_ASSERT(NS_IsMainThread());
    while (!mFinished) {
      NS_ProcessNextEvent(nullptr, true);
    }
    mFinished = false;
  }

private:
  void MarkFinished()
  {
    mFinished = true;
  }

public:
  void SetFinished()
  {
    NS_DispatchToMainThread(NS_NewNonOwningRunnableMethod(this,
                                                          &GMPTestMonitor::MarkFinished));
  }

private:
  bool mFinished;
};

#endif 
