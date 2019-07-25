



































#ifndef PSMRunnable_h
#define PSMRunnable_h

#include "mozilla/Monitor.h"
#include "nsThreadUtils.h"

namespace mozilla { namespace psm {




class SyncRunnableBase : public nsRunnable
{
public:
  NS_DECL_NSIRUNNABLE
  nsresult DispatchToMainThreadAndWait();
protected:
  SyncRunnableBase();
  virtual void RunOnTargetThread() = 0;
private:
  mozilla::Monitor monitor;
};

} } 

#endif
