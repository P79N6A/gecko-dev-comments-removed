









































#ifndef nsAppShell_h_
#define nsAppShell_h_

class nsCocoaWindow;

#include "nsBaseAppShell.h"
#include "nsTArray.h"

typedef struct _nsCocoaAppModalWindowListItem {
  _nsCocoaAppModalWindowListItem(NSWindow *aWindow, NSModalSession aSession) :
    mWindow(aWindow), mSession(aSession), mWidget(nsnull) {}
  _nsCocoaAppModalWindowListItem(NSWindow *aWindow, nsCocoaWindow *aWidget) :
    mWindow(aWindow), mSession(nil), mWidget(aWidget) {}
  NSWindow *mWindow;       
  NSModalSession mSession; 
  nsCocoaWindow *mWidget;  
} nsCocoaAppModalWindowListItem;

class nsCocoaAppModalWindowList {
public:
  nsCocoaAppModalWindowList() {}
  ~nsCocoaAppModalWindowList() {}
  
  nsresult PushCocoa(NSWindow *aWindow, NSModalSession aSession);
  
  nsresult PopCocoa(NSWindow *aWindow, NSModalSession aSession);
  
  nsresult PushGecko(NSWindow *aWindow, nsCocoaWindow *aWidget);
  
  nsresult PopGecko(NSWindow *aWindow, nsCocoaWindow *aWidget);
  
  NSModalSession CurrentSession();
  
  PRBool GeckoModalAboveCocoaModal();
private:
  nsTArray<nsCocoaAppModalWindowListItem> mList;
};


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

  PRBool InGeckoMainEventLoop();

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
  PRPackedBool       mNotifiedWillTerminate;
  PRPackedBool       mSkippedNativeCallback;
  PRPackedBool       mRunningCocoaEmbedded;

  
  
  PRUint32               mHadMoreEventsCount;
  
  
  
  
  
  static const PRUint32  kHadMoreEventsCountMax = 3;

  PRInt32            mRecursionDepth;
  PRInt32            mNativeEventCallbackDepth;
  
  PRInt32            mNativeEventScheduledDepth;
};

#endif 
