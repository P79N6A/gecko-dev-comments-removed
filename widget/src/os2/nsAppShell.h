




































#ifndef nsAppShell_h__
#define nsAppShell_h__

#include "nsBaseAppShell.h"
#define INCL_DEV
#define INCL_WIN
#define INCL_DOS
#include <os2.h>





class nsAppShell : public nsBaseAppShell
{
public:
  nsAppShell() : mEventWnd(NULL) {}

  nsresult Init();

protected:
  virtual void ScheduleNativeEventCallback();
  virtual PRBool ProcessNextNativeEvent(PRBool mayWait);
  virtual ~nsAppShell();

protected:
  HWND mEventWnd;

friend MRESULT EXPENTRY EventWindowProc(HWND, ULONG, MPARAM, MPARAM);
};


#endif 

