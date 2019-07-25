





#ifndef mozilla_lazyidlethread_h__
#define mozilla_lazyidlethread_h__

#ifndef MOZILLA_INTERNAL_API
#error "This header is only usable from within libxul (MOZILLA_INTERNAL_API)."
#endif

#include "nsIObserver.h"
#include "nsIThreadInternal.h"
#include "nsITimer.h"

#include "mozilla/Mutex.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsString.h"
#include "mozilla/Attributes.h"

#define IDLE_THREAD_TOPIC "thread-shutting-down"

namespace mozilla {








class LazyIdleThread MOZ_FINAL : public nsIThread,
                                 public nsITimerCallback,
                                 public nsIThreadObserver,
                                 public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIEVENTTARGET
  NS_DECL_NSITHREAD
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_NSITHREADOBSERVER
  NS_DECL_NSIOBSERVER

  enum ShutdownMethod {
    AutomaticShutdown = 0,
    ManualShutdown
  };

  



  LazyIdleThread(PRUint32 aIdleTimeoutMS,
                 const nsCSubstring& aName,
                 ShutdownMethod aShutdownMethod = AutomaticShutdown,
                 nsIObserver* aIdleObserver = nullptr);

  








  void SetWeakIdleObserver(nsIObserver* aObserver);

  



  void DisableIdleTimeout();

  


  void EnableIdleTimeout();

private:
  


  ~LazyIdleThread();

  


  void PreDispatch();

  


  nsresult EnsureThread();

  


  void InitThread();

  


  void CleanupThread();

  



  void ScheduleTimer();

  


  nsresult ShutdownThread();

  



  void SelfDestruct();

  



  bool UseRunnableQueue() {
    return !!mQueuedRunnables;
  }

  


  mozilla::Mutex mMutex;

  



  nsCOMPtr<nsIThread> mOwningThread;

  


  nsCOMPtr<nsIThread> mThread;

  




  nsCOMPtr<nsITimer> mIdleTimer;

  



  nsIObserver* mIdleObserver;

  



  nsTArray<nsCOMPtr<nsIRunnable> >* mQueuedRunnables;

  


  const PRUint32 mIdleTimeoutMS;

  



  PRUint32 mPendingEventCount;

  




  PRUint32 mIdleNotificationCount;

  




  ShutdownMethod mShutdownMethod;

  



  bool mShutdown;

  



  bool mThreadIsShuttingDown;

  


  bool mIdleTimeoutEnabled;

  


  nsCString mName;
};

} 

#endif
