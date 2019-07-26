









#ifndef nsAppShell_h_
#define nsAppShell_h_

class nsCocoaWindow;

#include "nsBaseAppShell.h"
#include "nsTArray.h"

typedef struct _nsCocoaAppModalWindowListItem {
  _nsCocoaAppModalWindowListItem(NSWindow *aWindow, NSModalSession aSession) :
    mWindow(aWindow), mSession(aSession), mWidget(nullptr) {}
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
  
  bool GeckoModalAboveCocoaModal();
private:
  nsTArray<nsCocoaAppModalWindowListItem> mList;
};




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
                                   uint32_t aRecursionDepth);

  
  void WillTerminate();

protected:
  virtual ~nsAppShell();

  virtual void ScheduleNativeEventCallback();
  virtual bool ProcessNextNativeEvent(bool aMayWait);

  bool InGeckoMainEventLoop();

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

  
  
  uint32_t               mHadMoreEventsCount;
  
  
  
  
  
  static const uint32_t  kHadMoreEventsCountMax = 3;

  int32_t            mRecursionDepth;
  int32_t            mNativeEventCallbackDepth;
  
  int32_t            mNativeEventScheduledDepth;
};

#endif 
