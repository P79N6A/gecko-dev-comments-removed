




#pragma once

#include "nsBaseAppShell.h"
#include <windows.h>
#include "nsWindowsHelpers.h"
#include "nsIObserver.h"

class MetroAppShell : public nsBaseAppShell
{
public:
  NS_DECL_NSIOBSERVER

  MetroAppShell() :
    mEventWnd(nullptr),
    mPowerRequestCount(0)
  {
  }

  nsresult Init();
  void DoProcessMoreGeckoEvents();
  void NativeCallback();

  static LRESULT CALLBACK EventWindowProc(HWND, UINT, WPARAM, LPARAM);
  static bool ProcessOneNativeEventIfPresent();
  static void MarkEventQueueForPurge();

protected:
  NS_IMETHOD Run();

  virtual void ScheduleNativeEventCallback();
  virtual bool ProcessNextNativeEvent(bool mayWait);
  static void DispatchAllGeckoEvents();
  virtual ~MetroAppShell();

  HWND mEventWnd;
  nsAutoHandle mPowerRequest;
  ULONG mPowerRequestCount;
};
