









































#ifndef nsAppShell_h_
#define nsAppShell_h_

#include "nsBaseAppShell.h"

@class AppShellDelegate;

class nsAppShell : public nsBaseAppShell
{
public:
  NS_IMETHODIMP ResumeNative(void);
	
  nsAppShell();

  nsresult Init();

  NS_IMETHOD Run(void);
  NS_IMETHOD OnProcessNextEvent(nsIThreadInternal *aThread, PRBool aMayWait,
                                PRUint32 aRecursionDepth);
  NS_IMETHOD AfterProcessNextEvent(nsIThreadInternal *aThread,
                                   PRUint32 aRecursionDepth);

  
  void ProcessGeckoEvents();
  void WillTerminate();

protected:
  virtual ~nsAppShell();

  virtual void ScheduleNativeEventCallback();
  virtual PRBool ProcessNextNativeEvent(PRBool aMayWait);

protected:
  NSAutoreleasePool* mMainPool;
  CFMutableArrayRef  mAutoreleasePools;

  NSPort*            mPort;
  AppShellDelegate*  mDelegate;

  PRPackedBool       mRunningEventLoop;
  PRPackedBool       mTerminated;
  PRPackedBool       mSkippedNativeCallback;
};

#endif 
