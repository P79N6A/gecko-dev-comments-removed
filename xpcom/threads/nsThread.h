





































#ifndef nsThread_h__
#define nsThread_h__

#include "nsIThreadInternal.h"
#include "nsISupportsPriority.h"
#include "nsEventQueue.h"
#include "nsThreadUtils.h"
#include "nsString.h"
#include "nsAutoLock.h"
#include "nsAutoPtr.h"


class nsThread : public nsIThreadInternal, public nsISupportsPriority
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIEVENTTARGET
  NS_DECL_NSITHREAD
  NS_DECL_NSITHREADINTERNAL
  NS_DECL_NSISUPPORTSPRIORITY

  nsThread();

  
  nsresult Init();

  
  nsresult InitCurrentThread();

  
  PRThread *GetPRThread() { return mThread; }

  
  
  PRBool ShutdownRequired() { return mShutdownRequired; }

private:
  friend class nsThreadShutdownEvent;

  ~nsThread();

  PRBool ShuttingDown() { return mShutdownContext != nsnull; }

  PR_STATIC_CALLBACK(void) ThreadFunc(void *arg);

  
  already_AddRefed<nsIThreadObserver> GetObserver() {
    nsIThreadObserver *obs;
    nsThread::GetObserver(&obs);
    return already_AddRefed<nsIThreadObserver>(obs);
  }

  
  PRBool GetEvent(PRBool mayWait, nsIRunnable **event) {
    return mEvents->GetEvent(mayWait, event);
  }
  PRBool PutEvent(nsIRunnable *event);

  
  class nsChainedEventQueue {
  public:
    nsChainedEventQueue(nsIThreadEventFilter *filter = nsnull)
      : mNext(nsnull), mFilter(filter) {
    }

    PRBool IsInitialized() {
      return mQueue.IsInitialized();
    }

    PRBool GetEvent(PRBool mayWait, nsIRunnable **event) {
      return mQueue.GetEvent(mayWait, event);
    }

    PRBool PutEvent(nsIRunnable *event);

    class nsChainedEventQueue *mNext;
  private:
    nsCOMPtr<nsIThreadEventFilter> mFilter;
    nsEventQueue mQueue;
  };

  
  
  
  
  
  PRLock *mLock;

  nsCOMPtr<nsIThreadObserver> mObserver;

  nsChainedEventQueue *mEvents;   
  nsChainedEventQueue  mEventsRoot;

  PRInt32   mPriority;
  PRThread *mThread;
  PRUint32  mRunningEvent;  

  struct nsThreadShutdownContext *mShutdownContext;

  PRPackedBool mShutdownRequired;
  PRPackedBool mShutdownPending;
};



class nsThreadSyncDispatch : public nsRunnable {
public:
  nsThreadSyncDispatch(nsIThread *origin, nsIRunnable *task)
    : mOrigin(origin), mSyncTask(task) {
  }

  PRBool IsPending() {
    return mSyncTask != nsnull;
  }

private:
  NS_DECL_NSIRUNNABLE

  nsCOMPtr<nsIThread> mOrigin;
  nsCOMPtr<nsIRunnable> mSyncTask;
};

#endif  
