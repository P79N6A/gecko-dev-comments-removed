




#ifndef nsBaseAppShell_h__
#define nsBaseAppShell_h__

#include "mozilla/Atomics.h"
#include "nsIAppShell.h"
#include "nsIThreadInternal.h"
#include "nsIObserver.h"
#include "nsIRunnable.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "prinrval.h"





class nsBaseAppShell : public nsIAppShell, public nsIThreadObserver,
                       public nsIObserver
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIAPPSHELL
  NS_DECL_NSITHREADOBSERVER
  NS_DECL_NSIOBSERVER

  nsBaseAppShell();

protected:
  virtual ~nsBaseAppShell();

  



  nsresult Init();

  


  void NativeEventCallback();

  




  virtual void DoProcessMoreGeckoEvents();

  



  virtual void ScheduleNativeEventCallback() = 0;

  











  virtual bool ProcessNextNativeEvent(bool mayWait) = 0;

  int32_t mSuspendNativeCount;
  uint32_t mEventloopNestingLevel;

private:
  bool DoProcessNextNativeEvent(bool mayWait, uint32_t recursionDepth);

  bool DispatchDummyEvent(nsIThread* target);

  void IncrementEventloopNestingLevel();
  void DecrementEventloopNestingLevel();

  


  void RunSyncSectionsInternal(bool stable, uint32_t threadRecursionLevel);

  void RunSyncSections(bool stable, uint32_t threadRecursionLevel)
  {
    if (!mSyncSections.IsEmpty()) {
      RunSyncSectionsInternal(stable, threadRecursionLevel);
    }
  }

  void ScheduleSyncSection(nsIRunnable* runnable, bool stable);

  struct SyncSection {
    SyncSection()
    : mStable(false), mEventloopNestingLevel(0), mThreadRecursionLevel(0)
    { }

    void Forget(SyncSection* other) {
      other->mStable = mStable;
      other->mEventloopNestingLevel = mEventloopNestingLevel;
      other->mThreadRecursionLevel = mThreadRecursionLevel;
      other->mRunnable = mRunnable.forget();
    }

    bool mStable;
    uint32_t mEventloopNestingLevel;
    uint32_t mThreadRecursionLevel;
    nsCOMPtr<nsIRunnable> mRunnable;
  };

  nsCOMPtr<nsIRunnable> mDummyEvent;
  





  bool *mBlockedWait;
  int32_t mFavorPerf;
  mozilla::Atomic<bool> mNativeEventPending;
  PRIntervalTime mStarvationDelay;
  PRIntervalTime mSwitchTime;
  PRIntervalTime mLastNativeEventTime;
  enum EventloopNestingState {
    eEventloopNone,  
    eEventloopXPCOM, 
    eEventloopOther  
  };
  EventloopNestingState mEventloopNestingState;
  nsTArray<SyncSection> mSyncSections;
  bool mRunningSyncSections;
  bool mRunning;
  bool mExiting;
  








  bool mBlockNativeEvent;
  














  bool mProcessedGeckoEvents;
};

#endif 
