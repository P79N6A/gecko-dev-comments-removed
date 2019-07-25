






































#ifndef mozilla_dom_indexeddb_lazyidlethread_h__
#define mozilla_dom_indexeddb_lazyidlethread_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIObserver.h"
#include "nsIThreadInternal.h"
#include "nsITimer.h"

#include "mozilla/Mutex.h"

#define IDLE_THREAD_TOPIC "thread-shutting-down"

BEGIN_INDEXEDDB_NAMESPACE








class LazyIdleThread : public nsIThread,
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
                 ShutdownMethod aShutdownMethod = AutomaticShutdown,
                 nsIObserver* aIdleObserver = nsnull);

  








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
};

END_INDEXEDDB_NAMESPACE

#endif
