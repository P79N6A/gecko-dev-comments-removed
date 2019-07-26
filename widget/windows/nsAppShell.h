




#ifndef nsAppShell_h__
#define nsAppShell_h__

#include "nsBaseAppShell.h"
#include <windows.h>
#include "mozilla/TimeStamp.h"
#include "mozilla/Mutex.h"



#define NATIVE_EVENT_STARVATION_LIMIT 1




class nsAppShell : public nsBaseAppShell
{
public:
  nsAppShell() :
    mEventWnd(NULL),
    mNativeCallbackPending(false),
    mLastNativeEventScheduledMutex("nsAppShell::mLastNativeEventScheduledMutex")
  {}
  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::Mutex Mutex;

  nsresult Init();
  void DoProcessMoreGeckoEvents();

  static UINT GetTaskbarButtonCreatedMessage();

protected:
  NS_IMETHOD Run();
  NS_IMETHOD Exit();
  virtual void ScheduleNativeEventCallback();
  virtual bool ProcessNextNativeEvent(bool mayWait);
  virtual ~nsAppShell();

  static LRESULT CALLBACK EventWindowProc(HWND, UINT, WPARAM, LPARAM);

protected:
  HWND mEventWnd;
  bool mNativeCallbackPending;

  Mutex mLastNativeEventScheduledMutex;
  TimeStamp mLastNativeEventScheduled;
};

#endif 
