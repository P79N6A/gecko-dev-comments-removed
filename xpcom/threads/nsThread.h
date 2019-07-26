





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


class nsThread MOZ_FINAL : public nsIThreadInternal,
                           public nsISupportsPriority
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

  nsThread(MainThreadFlag aMainThread, uint32_t aStackSize);

  
  nsresult Init();

  
  nsresult InitCurrentThread();

  
  PRThread *GetPRThread() { return mThread; }

  
  
  bool ShutdownRequired() { return mShutdownRequired; }

  
  void ClearObservers() { mEventObservers.Clear(); }

private:
  friend class nsThreadShutdownEvent;

  ~nsThread();

  bool ShuttingDown() { return mShutdownContext != nullptr; }

  static void ThreadFunc(void *arg);

  
  already_AddRefed<nsIThreadObserver> GetObserver() {
    nsIThreadObserver *obs;
    nsThread::GetObserver(&obs);
    return already_AddRefed<nsIThreadObserver>(obs);
  }

  
  bool GetEvent(bool mayWait, nsIRunnable **event) {
    return mEvents.GetEvent(mayWait, event);
  }
  nsresult PutEvent(nsIRunnable *event);

  
  
  
  
  
  mozilla::Mutex mLock;

  nsCOMPtr<nsIThreadObserver> mObserver;

  
  nsAutoTObserverArray<nsCOMPtr<nsIThreadObserver>, 2> mEventObservers;

  nsEventQueue  mEvents;

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

namespace mozilla {







void ScheduleMemoryPressureEvent();

} 

#endif  
