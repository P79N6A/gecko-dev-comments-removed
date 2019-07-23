




































#ifndef nsBaseAppShell_h__
#define nsBaseAppShell_h__

#include "nsIAppShell.h"
#include "nsIThreadInternal.h"
#include "nsIObserver.h"
#include "nsIRunnable.h"
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
  virtual ~nsBaseAppShell() {}

  



  nsresult Init();

  


  void NativeEventCallback();

  



  virtual void ScheduleNativeEventCallback() = 0;

  











  virtual PRBool ProcessNextNativeEvent(PRBool mayWait) = 0;

  PRInt32 mSuspendNativeCount;

private:
  PRBool DoProcessNextNativeEvent(PRBool mayWait);

  nsCOMPtr<nsIRunnable> mDummyEvent;
  





  PRBool *mBlockedWait;
  PRInt32 mFavorPerf;
  PRInt32 mNativeEventPending;
  PRUint32 mEventloopNestingLevel;
  PRIntervalTime mStarvationDelay;
  PRIntervalTime mSwitchTime;
  PRIntervalTime mLastNativeEventTime;
  enum EventloopNestingState {
    eEventloopNone,  
    eEventloopXPCOM, 
    eEventloopOther  
  };
  EventloopNestingState mEventloopNestingState;
  PRPackedBool mRunning;
  PRPackedBool mExiting;
  








  PRPackedBool mBlockNativeEvent;
};

#endif 
