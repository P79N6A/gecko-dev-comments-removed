











































#ifndef nsAppShell_h__
#define nsAppShell_h__

#include <CoreFoundation/CoreFoundation.h>

#include "nsBaseAppShell.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"

class nsIToolkit;
class nsMacMessagePump;

class nsAppShell : public nsBaseAppShell
{
public:
  nsAppShell();

  nsresult Init();

protected:
  virtual ~nsAppShell();

  virtual void ScheduleNativeEventCallback();
  virtual PRBool ProcessNextNativeEvent(PRBool aMayWait);

  static void ProcessGeckoEvents(void* aInfo);

protected:
  nsCOMPtr<nsIToolkit>        mToolkit;
  nsAutoPtr<nsMacMessagePump> mMacPump;

  CFRunLoopRef                mCFRunLoop;
  CFRunLoopSourceRef          mCFRunLoopSource;

  PRBool                      mRunningEventLoop;
};

#endif 
