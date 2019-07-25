





































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

  enum MainThreadFlag {
    MAIN_THREAD,
    NOT_MAIN_THREAD
  };

  nsThread(MainThreadFlag aMainThread, PRUint32 aStackSize);

  
  nsresult Init();

  
  nsresult InitCurrentThread();

  
  PRThread *GetPRThread() { return mThread; }

  
  
  bool ShutdownRequired() { return mShutdownRequired; }

  
  static nsIThreadObserver* sGlobalObserver;

private:
  friend class nsThreadShutdownEvent;

  ~nsThread();

  bool ShuttingDown() { return mShutdownContext != nsnull; }

  static void ThreadFunc(void *arg);

  
  already_AddRefed<nsIThreadObserver> GetObserver() {
    nsIThreadObserver *obs;
    nsThread::GetObserver(&obs);
    return already_AddRefed<nsIThreadObserver>(obs);
  }

  
  bool GetEvent(bool mayWait, nsIRunnable **event) {
    return mEvents->GetEvent(mayWait, event);
  }
  nsresult PutEvent(nsIRunnable *event);

  
  class nsChainedEventQueue {
  public:
    nsChainedEventQueue(nsIThreadEventFilter *filter = nsnull)
      : mNext(nsnull), mFilter(filter) {
    }

    bool GetEvent(bool mayWait, nsIRunnable **event) {
      return mQueue.GetEvent(mayWait, event);
    }

    bool PutEvent(nsIRunnable *event);
    
    bool HasPendingEvent() {
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
  PRUint32  mStackSize;

  struct nsThreadShutdownContext *mShutdownContext;

  bool mShutdownRequired;
  bool mShutdownPending;
  
  bool mEventsAreDoomed;
  MainThreadFlag mIsMainThread;
};



class nsThreadSyncDispatch : public nsRunnable {
public:
  nsThreadSyncDispatch(nsIThread *origin, nsIRunnable *task)
    : mOrigin(origin), mSyncTask(task), mResult(NS_ERROR_NOT_INITIALIZED) {
  }

  bool IsPending() {
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
