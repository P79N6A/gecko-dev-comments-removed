






































#ifndef mozilla_dom_indexeddb_lazyidlethread_h__
#define mozilla_dom_indexeddb_lazyidlethread_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIObserver.h"
#include "nsIThreadInternal.h"
#include "nsITimer.h"

#include "mozilla/Mutex.h"

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

  



  LazyIdleThread(PRUint32 aIdleTimeoutMS);

  


  void EnableIdleTimeout(PRBool aEnable);

private:
  


  ~LazyIdleThread();

  


  nsresult EnsureThread();

  


  void InitThread();

  


  void ShutdownThread();

  


  void CancelTimer(nsITimer* aTimer);

  


  mozilla::Mutex mMutex;

  



  nsCOMPtr<nsIThread> mOwningThread;

  


  const PRUint32 mIdleTimeoutMS;

  


  nsCOMPtr<nsIThread> mThread;

  



  PRBool mShutdown;

  




  nsCOMPtr<nsITimer> mIdleTimer;

  



  PRBool mThreadHasTimedOut;

  


  PRUint32 mTimeoutDisabledCount;
};

END_INDEXEDDB_NAMESPACE

#endif 
