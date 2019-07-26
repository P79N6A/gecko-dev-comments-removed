





#ifndef nsThread_h__
#define nsThread_h__

#include "mozilla/Mutex.h"
#include "nsIThreadInternal.h"
#include "nsISupportsPriority.h"
#include "nsEventQueue.h"
#include "nsThreadUtils.h"
#include "nsString.h"
#include "nsTObserverArray.h"
#include "mozilla/Attributes.h"
#include "nsAutoPtr.h"


class nsThread : public nsIThreadInternal,
                 public nsISupportsPriority
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIEVENTTARGET
  NS_DECL_NSITHREAD
  NS_DECL_NSITHREADINTERNAL
  NS_DECL_NSISUPPORTSPRIORITY

  enum MainThreadFlag {
    MAIN_THREAD,
    NOT_MAIN_THREAD
  };

  nsThread(MainThreadFlag aMainThread, uint32_t aStackSize);

  
  nsresult Init();

  
  nsresult InitCurrentThread();

  
  PRThread *GetPRThread() { return mThread; }

  
  
  bool ShutdownRequired() { return mShutdownRequired; }

  
  void ClearObservers() { mEventObservers.Clear(); }

  static nsresult
  SetMainThreadObserver(nsIThreadObserver* aObserver);

protected:
  static nsIThreadObserver* sMainThreadObserver;

  class nsChainedEventQueue;

  class nsNestedEventTarget;
  friend class nsNestedEventTarget;

  friend class nsThreadShutdownEvent;

  virtual ~nsThread();

  bool ShuttingDown() { return mShutdownContext != nullptr; }

  static void ThreadFunc(void *arg);

  
  already_AddRefed<nsIThreadObserver> GetObserver() {
    nsIThreadObserver *obs;
    nsThread::GetObserver(&obs);
    return already_AddRefed<nsIThreadObserver>(obs);
  }

  
  bool GetEvent(bool mayWait, nsIRunnable **event) {
    return mEvents->GetEvent(mayWait, event);
  }
  nsresult PutEvent(nsIRunnable *event, nsNestedEventTarget *target);

  nsresult DispatchInternal(nsIRunnable *event, uint32_t flags,
                            nsNestedEventTarget *target);

  
  class nsChainedEventQueue {
  public:
    nsChainedEventQueue()
      : mNext(nullptr) {
    }

    bool GetEvent(bool mayWait, nsIRunnable **event) {
      return mQueue.GetEvent(mayWait, event);
    }

    void PutEvent(nsIRunnable *event) {
      mQueue.PutEvent(event);
    }

    bool HasPendingEvent() {
      return mQueue.HasPendingEvent();
    }

    nsChainedEventQueue *mNext;
    nsRefPtr<nsNestedEventTarget> mEventTarget;

  private:
    nsEventQueue mQueue;
  };

  class nsNestedEventTarget MOZ_FINAL : public nsIEventTarget {
  public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIEVENTTARGET

    nsNestedEventTarget(nsThread *thread, nsChainedEventQueue *queue)
      : mThread(thread), mQueue(queue) {
    }

    nsRefPtr<nsThread> mThread;

    
    nsChainedEventQueue* mQueue;

  private:
    ~nsNestedEventTarget() {}
  };

  
  
  
  
  
  mozilla::Mutex mLock;

  nsCOMPtr<nsIThreadObserver> mObserver;

  
  nsAutoTObserverArray<nsCOMPtr<nsIThreadObserver>, 2> mEventObservers;

  nsChainedEventQueue *mEvents;   
  nsChainedEventQueue  mEventsRoot;

  int32_t   mPriority;
  PRThread *mThread;
  uint32_t  mRunningEvent;  
  uint32_t  mStackSize;

  struct nsThreadShutdownContext *mShutdownContext;

  bool mShutdownRequired;
  
  bool mEventsAreDoomed;
  MainThreadFlag mIsMainThread;
};



class nsThreadSyncDispatch : public nsRunnable {
public:
  nsThreadSyncDispatch(nsIThread *origin, nsIRunnable *task)
    : mOrigin(origin), mSyncTask(task), mResult(NS_ERROR_NOT_INITIALIZED) {
  }

  bool IsPending() {
    return mSyncTask != nullptr;
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

#if defined(XP_UNIX) && !defined(ANDROID) && !defined(DEBUG) && HAVE_UALARM \
  && defined(_GNU_SOURCE)
# define MOZ_CANARY

extern int sCanaryOutputFD;
#endif

#endif  
