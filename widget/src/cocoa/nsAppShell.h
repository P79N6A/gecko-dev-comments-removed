









































#ifndef nsAppShell_h_
#define nsAppShell_h_

#include "nsBaseAppShell.h"

@class AppShellDelegate;

class nsAppShell : public nsBaseAppShell
{
public:
  NS_IMETHOD ResumeNative(void);
	
  nsAppShell();

  nsresult Init();

  NS_IMETHOD Run(void);
  NS_IMETHOD Exit(void);
  NS_IMETHOD OnProcessNextEvent(nsIThreadInternal *aThread, PRBool aMayWait,
                                PRUint32 aRecursionDepth);
  NS_IMETHOD AfterProcessNextEvent(nsIThreadInternal *aThread,
                                   PRUint32 aRecursionDepth);

  
  void WillTerminate();

protected:
  virtual ~nsAppShell();

  virtual void ScheduleNativeEventCallback();
  virtual PRBool ProcessNextNativeEvent(PRBool aMayWait);

  static void ProcessGeckoEvents(void* aInfo);

protected:
  NSAutoreleasePool* mMainPool;
  CFMutableArrayRef  mAutoreleasePools;

  AppShellDelegate*  mDelegate;
  CFRunLoopRef       mCFRunLoop;
  CFRunLoopSourceRef mCFRunLoopSource;

  PRPackedBool       mRunningEventLoop;
  PRPackedBool       mStarted;
  PRPackedBool       mTerminated;
  PRPackedBool       mSkippedNativeCallback;

  
  
  PRUint32               mHadMoreEventsCount;
  
  
  
  
  
  static const PRUint32  kHadMoreEventsCountMax = 3;
};

#endif 
