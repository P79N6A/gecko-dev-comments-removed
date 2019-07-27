





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
#include "mozilla/ReentrantMonitor.h"


class nsThread
  : public nsIThreadInternal
  , public nsISupportsPriority
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIEVENTTARGET
  NS_DECL_NSITHREAD
  NS_DECL_NSITHREADINTERNAL
  NS_DECL_NSISUPPORTSPRIORITY

  enum MainThreadFlag
  {
    MAIN_THREAD,
    NOT_MAIN_THREAD
  };

  nsThread(MainThreadFlag aMainThread, uint32_t aStackSize);

  
  nsresult Init();

  
  nsresult InitCurrentThread();

  
  PRThread* GetPRThread()
  {
    return mThread;
  }

  
  
  bool ShutdownRequired()
  {
    return mShutdownRequired;
  }

  
  void ClearObservers()
  {
    mEventObservers.Clear();
  }

  static nsresult
  SetMainThreadObserver(nsIThreadObserver* aObserver);

#ifdef MOZ_NUWA_PROCESS
  void SetWorking();
  void SetIdle();
  mozilla::ReentrantMonitor& ThreadStatusMonitor() {
    return mThreadStatusMonitor;
  }
#endif

protected:
  static nsIThreadObserver* sMainThreadObserver;

  class nsChainedEventQueue;

  class nsNestedEventTarget;
  friend class nsNestedEventTarget;

  friend class nsThreadShutdownEvent;

  virtual ~nsThread();

  bool ShuttingDown()
  {
    return mShutdownContext != nullptr;
  }

  static void ThreadFunc(void* aArg);

  
  already_AddRefed<nsIThreadObserver> GetObserver()
  {
    nsIThreadObserver* obs;
    nsThread::GetObserver(&obs);
    return already_AddRefed<nsIThreadObserver>(obs);
  }

  
  bool GetEvent(bool aMayWait, nsIRunnable** aEvent)
  {
    return mEvents->GetEvent(aMayWait, aEvent);
  }
  nsresult PutEvent(nsIRunnable* aEvent, nsNestedEventTarget* aTarget);

  nsresult DispatchInternal(nsIRunnable* aEvent, uint32_t aFlags,
                            nsNestedEventTarget* aTarget);

  
  class nsChainedEventQueue
  {
  public:
    nsChainedEventQueue()
      : mNext(nullptr)
    {
    }

    bool GetEvent(bool aMayWait, nsIRunnable** aEvent)
    {
      return mQueue.GetEvent(aMayWait, aEvent);
    }

    void PutEvent(nsIRunnable* aEvent)
    {
      mQueue.PutEvent(aEvent);
    }

    bool HasPendingEvent()
    {
      return mQueue.HasPendingEvent();
    }

    nsChainedEventQueue* mNext;
    nsRefPtr<nsNestedEventTarget> mEventTarget;

  private:
    nsEventQueue mQueue;
  };

  class nsNestedEventTarget final : public nsIEventTarget
  {
  public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIEVENTTARGET

    nsNestedEventTarget(nsThread* aThread, nsChainedEventQueue* aQueue)
      : mThread(aThread)
      , mQueue(aQueue)
    {
    }

    nsRefPtr<nsThread> mThread;

    
    nsChainedEventQueue* mQueue;

  private:
    ~nsNestedEventTarget()
    {
    }
  };

  
  
  
  
  
  mozilla::Mutex mLock;

  nsCOMPtr<nsIThreadObserver> mObserver;

  
  nsAutoTObserverArray<nsCOMPtr<nsIThreadObserver>, 2> mEventObservers;

  nsChainedEventQueue* mEvents;  
  nsChainedEventQueue  mEventsRoot;

  int32_t   mPriority;
  PRThread* mThread;
  uint32_t  mNestedEventLoopDepth;
  uint32_t  mStackSize;

  struct nsThreadShutdownContext* mShutdownContext;

  bool mShutdownRequired;
  
  bool mEventsAreDoomed;
  MainThreadFlag mIsMainThread;
#ifdef MOZ_NUWA_PROCESS
  mozilla::ReentrantMonitor mThreadStatusMonitor;
  
  
  void* mThreadStatusInfo;
#endif
};



class nsThreadSyncDispatch : public nsRunnable
{
public:
  nsThreadSyncDispatch(nsIThread* aOrigin, nsIRunnable* aTask)
    : mOrigin(aOrigin)
    , mSyncTask(aTask)
    , mResult(NS_ERROR_NOT_INITIALIZED)
  {
  }

  bool IsPending()
  {
    return mSyncTask != nullptr;
  }

  nsresult Result()
  {
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
