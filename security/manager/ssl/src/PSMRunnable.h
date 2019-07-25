



































#ifndef PSMRunnable_h
#define PSMRunnable_h

#include "mozilla/Monitor.h"
#include "nsThreadUtils.h"
#include "nsIObserver.h"

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

class NotifyObserverRunnable : public nsRunnable
{
public:
  NotifyObserverRunnable(nsIObserver * observer,
                         const char * topicStringLiteral)
    : mObserver(), mTopic(topicStringLiteral) { mObserver = observer; }
  NS_DECL_NSIRUNNABLE
private:
  nsCOMPtr<nsIObserver> mObserver;
  const char * const mTopic;
};

} } 

#endif
