





































#ifndef nsThread_h__
#define nsThread_h__

#include "mozilla/Mutex.h"
#include "nsIThreadInternal.h"
#include "nsISupportsPriority.h"
#include "nsEventQueue.h"
#include "nsThreadUtils.h"
#include "nsString.h"
#include "nsTObserverArray.h"


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

  
  static nsIThreadObserver* sGlobalObserver;

private:
  friend class nsThreadShutdownEvent;

  ~nsThread();

  PRBool ShuttingDown() { return mShutdownContext != nsnull; }

  static void ThreadFunc(void *arg);

  
  already_AddRefed<nsIThreadObserver> GetObserver() {
    nsIThreadObserver *obs;
    nsThread::GetObserver(&obs);
    return already_AddRefed<nsIThreadObserver>(obs);
  }

  
  PRBool GetEvent(PRBool mayWait, nsIRunnable **event) {
    return mEvents->GetEvent(mayWait, event);
  }
  nsresult PutEvent(nsIRunnable *event);

  
  class nsChainedEventQueue {
  public:
    nsChainedEventQueue(nsIThreadEventFilter *filter = nsnull)
      : mNext(nsnull), mFilter(filter) {
    }

    PRBool GetEvent(PRBool mayWait, nsIRunnable **event) {
      return mQueue.GetEvent(mayWait, event);
    }

    PRBool PutEvent(nsIRunnable *event);
    
    PRBool HasPendingEvent() {
      return mQueue.HasPendingEvent();
    }

    class nsChainedEventQueue *mNext;
  private:
    nsCOMPtr<nsIThreadEventFilter> mFilter;
    nsEventQueue mQueue;
  };

  
  
  
  
  
  mozilla::Mutex mLock;

  nsCOMPtr<nsIThreadObserver> mObserver;

  
  nsAutoTObserverArray<nsCOMPtr<nsIThreadObserver>, 2> mEventObservers;

  nsChainedEventQueue *mEvents;   
  nsChainedEventQueue  mEventsRoot;

  PRInt32   mPriority;
  PRThread *mThread;
  PRUint32  mRunningEvent;  

  struct nsThreadShutdownContext *mShutdownContext;

  PRPackedBool mShutdownRequired;
  PRPackedBool mShutdownPending;
  
  PRPackedBool mEventsAreDoomed;
};



class nsThreadSyncDispatch : public nsRunnable {
public:
  nsThreadSyncDispatch(nsIThread *origin, nsIRunnable *task)
    : mOrigin(origin), mSyncTask(task), mResult(NS_ERROR_NOT_INITIALIZED) {
  }

  PRBool IsPending() {
    return mSyncTask != nsnull;
  }

  nsresult Result() {
    return mResult;
  }

private:
  NS_DECL_NSIRUNNABLE

  nsCOMPtr<nsIThread> mOrigin;
  nsCOMPtr<nsIRunnable> mSyncTask;
  nsresult mResult;
};

#endif  
