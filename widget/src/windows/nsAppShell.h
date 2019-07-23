




































#ifndef nsAppShell_h__
#define nsAppShell_h__

#include "nsBaseAppShell.h"
#include <windows.h>




class nsAppShell : public nsBaseAppShell
{
public:
  nsAppShell() : mEventWnd(NULL) {}

  nsresult Init();

#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_WIN7
  static UINT GetTaskbarButtonCreatedMessage();
#endif

protected:
  virtual void ScheduleNativeEventCallback();
  virtual PRBool ProcessNextNativeEvent(PRBool mayWait);
  virtual ~nsAppShell();

  static LRESULT CALLBACK EventWindowProc(HWND, UINT, WPARAM, LPARAM);

protected:
  HWND mEventWnd;
};

#endif 
