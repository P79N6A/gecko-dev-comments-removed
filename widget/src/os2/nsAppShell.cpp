






































#include "nsAppShell.h"
#include "nsToolkit.h"
#include "nsThreadUtils.h"

static UINT sMsgId;



MRESULT EXPENTRY EventWindowProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  if (msg == sMsgId) {
    nsAppShell *as = reinterpret_cast<nsAppShell *>(mp2);
    as->NativeEventCallback();
    NS_RELEASE(as);
    return (MRESULT)TRUE;
  }
  return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

nsAppShell::~nsAppShell()
{
  if (mEventWnd) {
    
    
    
    WinSendMsg(mEventWnd, WM_CLOSE, 0, 0);
  }
}

nsresult
nsAppShell::Init()
{
  
  
  if (WinQueryQueueInfo(HMQ_CURRENT, NULL, 0) == FALSE) {
    
    PPIB ppib;
    PTIB ptib;
    DosGetInfoBlocks(&ptib, &ppib);
    ppib->pib_ultype = 3;

    HAB hab = WinInitialize(0);
    WinCreateMsgQueue(hab, 0);
  }

  if (!sMsgId) {
    sMsgId = WinAddAtom( WinQuerySystemAtomTable(), "nsAppShell:EventID");
    WinRegisterClass((HAB)0, "nsAppShell:EventWindowClass", EventWindowProc, NULL, 0);
  }

  mEventWnd = ::WinCreateWindow(HWND_DESKTOP,
                                "nsAppShell:EventWindowClass",
                                "nsAppShell:EventWindow",
                                0,
                                0, 0,
                                10, 10,
                                HWND_DESKTOP,
                                HWND_BOTTOM,
                                0, 0, 0);
  NS_ENSURE_STATE(mEventWnd);

  return nsBaseAppShell::Init();
}

void
nsAppShell::ScheduleNativeEventCallback()
{
  
  NS_ADDREF_THIS();
  WinPostMsg(mEventWnd, sMsgId, 0, reinterpret_cast<MPARAM>(this));
}

PRBool
nsAppShell::ProcessNextNativeEvent(PRBool mayWait)
{
  PRBool gotMessage = PR_FALSE;

  do {
    QMSG qmsg;
    
    
    if (WinPeekMsg((HAB)0, &qmsg, NULL, WM_CHAR, WM_VIOCHAR, PM_REMOVE) ||
        WinPeekMsg((HAB)0, &qmsg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE) || 
        WinPeekMsg((HAB)0, &qmsg, NULL, 0, WM_USER-1, PM_REMOVE) || 
        WinPeekMsg((HAB)0, &qmsg, NULL, 0, 0, PM_REMOVE)) {
      gotMessage = PR_TRUE;
      ::WinDispatchMsg((HAB)0, &qmsg);
    } else if (mayWait) {
      
      ::WinWaitMsg((HAB)0, 0, 0);
    }
  } while (!gotMessage && mayWait);

  return gotMessage;
}
