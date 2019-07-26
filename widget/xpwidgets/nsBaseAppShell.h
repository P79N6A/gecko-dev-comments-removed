




































#ifndef nsBaseAppShell_h__
#define nsBaseAppShell_h__

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
  NS_DECL_ISUPPORTS
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

  PRInt32 mSuspendNativeCount;
  PRUint32 mEventloopNestingLevel;

private:
  bool DoProcessNextNativeEvent(bool mayWait, PRUint32 recursionDepth);

  bool DispatchDummyEvent(nsIThread* target);

  


  void RunSyncSectionsInternal(bool stable, PRUint32 threadRecursionLevel);

  void RunSyncSections(bool stable, PRUint32 threadRecursionLevel)
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
    PRUint32 mEventloopNestingLevel;
    PRUint32 mThreadRecursionLevel;
    nsCOMPtr<nsIRunnable> mRunnable;
  };

  nsCOMPtr<nsIRunnable> mDummyEvent;
  





  bool *mBlockedWait;
  PRInt32 mFavorPerf;
  PRInt32 mNativeEventPending;
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
  bool mRunning;
  bool mExiting;
  








  bool mBlockNativeEvent;
  














  bool mProcessedGeckoEvents;
};

#endif 
