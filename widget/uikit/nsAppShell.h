









#ifndef nsAppShell_h_
#define nsAppShell_h_

#include "nsBaseAppShell.h"
#include "nsTArray.h"

#include <Foundation/NSAutoreleasePool.h>
#include <CoreFoundation/CFRunLoop.h>
#include <UIKit/UIWindow.h>

@class AppShellDelegate;

class nsAppShell : public nsBaseAppShell
{
public:
  NS_IMETHOD ResumeNative(void);

  nsAppShell();

  nsresult Init();

  NS_IMETHOD Run(void);
  NS_IMETHOD Exit(void);
  
  void WillTerminate(void);

  static nsAppShell* gAppShell;
  static UIWindow* gWindow;
  static NSMutableArray* gTopLevelViews;

protected:
  virtual ~nsAppShell();

  static void ProcessGeckoEvents(void* aInfo);
  virtual void ScheduleNativeEventCallback();
  virtual bool ProcessNextNativeEvent(bool aMayWait);

  NSAutoreleasePool* mAutoreleasePool;
  AppShellDelegate*  mDelegate;
  CFRunLoopRef       mCFRunLoop;
  CFRunLoopSourceRef mCFRunLoopSource;

  bool               mTerminated;
  bool               mNotifiedWillTerminate;
};

#endif 
