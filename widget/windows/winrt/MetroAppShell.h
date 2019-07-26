




#pragma once

#include "nsBaseAppShell.h"
#include <windows.h>
#include "mozilla/TimeStamp.h"
#include "nsWindowsHelpers.h"
#include "nsIObserver.h"

class MetroAppShell : public nsBaseAppShell
{
public:
  NS_DECL_NSIOBSERVER

  MetroAppShell() : mEventWnd(NULL), mExiting(false), mPowerRequestCount(0)
  {}

  nsresult Init();
  void DoProcessMoreGeckoEvents();
  void NativeCallback();

  static LRESULT CALLBACK EventWindowProc(HWND, UINT, WPARAM, LPARAM);
  static void ProcessAllNativeEventsPresent();
  static void ProcessOneNativeEventIfPresent();

protected:
  HWND mEventWnd;
  bool mExiting;
  nsAutoHandle mPowerRequest;
  ULONG mPowerRequestCount;

  NS_IMETHOD Run();
  NS_IMETHOD Exit();
  virtual void ScheduleNativeEventCallback();
  virtual bool ProcessNextNativeEvent(bool mayWait);
  virtual ~MetroAppShell();
};
