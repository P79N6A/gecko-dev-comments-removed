




#ifndef nsAppShell_h__
#define nsAppShell_h__

#include "nsBaseAppShell.h"
#include <windows.h>
#include "mozilla/TimeStamp.h"


#define NATIVE_EVENT_STARVATION_LIMIT mozilla::TimeDuration::FromSeconds(1)




class nsAppShell : public nsBaseAppShell
{
public:
  nsAppShell() :
    mEventWnd(NULL),
    mNativeCallbackPending(false)
  {}
  typedef mozilla::TimeStamp TimeStamp;

  nsresult Init();
  void DoProcessMoreGeckoEvents();

  static UINT GetTaskbarButtonCreatedMessage();

protected:
#if defined(_MSC_VER) && defined(_M_IX86)
  NS_IMETHOD Run();
#endif
  NS_IMETHOD Exit();
  virtual void ScheduleNativeEventCallback();
  virtual bool ProcessNextNativeEvent(bool mayWait);
  virtual ~nsAppShell();

  static LRESULT CALLBACK EventWindowProc(HWND, UINT, WPARAM, LPARAM);

protected:
  HWND mEventWnd;
  bool mNativeCallbackPending;
  TimeStamp mLastNativeEventScheduled;
};

#endif 
