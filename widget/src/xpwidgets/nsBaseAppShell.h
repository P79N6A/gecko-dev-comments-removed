




































#ifndef nsBaseAppShell_h__
#define nsBaseAppShell_h__

#include "nsIAppShell.h"
#include "nsIThreadInternal.h"
#include "nsIObserver.h"
#include "nsIRunnable.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
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

  











  virtual PRBool ProcessNextNativeEvent(PRBool mayWait) = 0;

  PRInt32 mSuspendNativeCount;
  PRUint32 mEventloopNestingLevel;

private:
  PRBool DoProcessNextNativeEvent(PRBool mayWait);

  


  void RunSyncSections();

  nsCOMPtr<nsIRunnable> mDummyEvent;
  





  PRBool *mBlockedWait;
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
  nsCOMArray<nsIRunnable> mSyncSections;
  PRPackedBool mRunning;
  PRPackedBool mExiting;
  








  PRPackedBool mBlockNativeEvent;
  














  PRPackedBool mProcessedGeckoEvents;
};

#endif 
