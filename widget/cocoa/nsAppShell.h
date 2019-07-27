









#ifndef nsAppShell_h_
#define nsAppShell_h_

#include "nsBaseAppShell.h"
#include "nsTArray.h"




@interface GeckoNSApplication : NSApplication
{
}
@end

@class AppShellDelegate;

class nsAppShell : public nsBaseAppShell
{
public:
  NS_IMETHOD ResumeNative(void);

  nsAppShell();

  nsresult Init();

  NS_IMETHOD Run(void);
  NS_IMETHOD Exit(void);
  NS_IMETHOD OnProcessNextEvent(nsIThreadInternal *aThread, bool aMayWait,
                                uint32_t aRecursionDepth);
  NS_IMETHOD AfterProcessNextEvent(nsIThreadInternal *aThread,
                                   uint32_t aRecursionDepth,
                                   bool aEventWasProcessed);

  
  void WillTerminate();

protected:
  virtual ~nsAppShell();

  virtual void ScheduleNativeEventCallback();
  virtual bool ProcessNextNativeEvent(bool aMayWait);

  static void ProcessGeckoEvents(void* aInfo);

protected:
  CFMutableArrayRef  mAutoreleasePools;

  AppShellDelegate*  mDelegate;
  CFRunLoopRef       mCFRunLoop;
  CFRunLoopSourceRef mCFRunLoopSource;

  bool               mRunningEventLoop;
  bool               mStarted;
  bool               mTerminated;
  bool               mSkippedNativeCallback;
  bool               mRunningCocoaEmbedded;

  int32_t            mNativeEventCallbackDepth;
  
  int32_t            mNativeEventScheduledDepth;
};

#endif 
