






































#include "nsAppShell.h"
#include "nsToolkit.h"
#include "nsThreadUtils.h"

#ifdef WINCE
BOOL WaitMessage(VOID)
{
  BOOL retval = TRUE;
  
  HANDLE hThread = GetCurrentThread();
  DWORD waitRes = MsgWaitForMultipleObjectsEx(1, &hThread, INFINITE, QS_ALLEVENTS, 0);
  if((DWORD)-1 == waitRes)
  {
    retval = FALSE;
  }
  
  return retval;
}
#endif

static UINT sMsgId;

#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_WIN7
static UINT sTaskbarButtonCreatedMsg;


UINT nsAppShell::GetTaskbarButtonCreatedMessage() {
	return sTaskbarButtonCreatedMsg;
}
#endif



static BOOL PeekKeyAndIMEMessage(LPMSG msg, HWND hwnd)
{
  MSG msg1, msg2, *lpMsg;
  BOOL b1, b2;
  b1 = ::PeekMessageW(&msg1, NULL, WM_KEYFIRST, WM_IME_KEYLAST, PM_NOREMOVE);
  b2 = ::PeekMessageW(&msg2, NULL, WM_IME_SETCONTEXT, WM_IME_KEYUP, PM_NOREMOVE);
  if (b1 || b2) {
    if (b1 && b2) {
      if (msg1.time < msg2.time)
        lpMsg = &msg1;
      else
        lpMsg = &msg2;
    } else if (b1)
      lpMsg = &msg1;
    else
      lpMsg = &msg2;
    return ::PeekMessageW(msg, hwnd, lpMsg->message, lpMsg->message, PM_REMOVE);
  }

  return false;
}

 LRESULT CALLBACK
nsAppShell::EventWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if (uMsg == sMsgId) {
    nsAppShell *as = reinterpret_cast<nsAppShell *>(lParam);
    as->NativeEventCallback();
    NS_RELEASE(as);
    return TRUE;
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

nsAppShell::~nsAppShell()
{
  if (mEventWnd) {
    
    
    
    SendMessage(mEventWnd, WM_CLOSE, 0, 0);
  }
}

nsresult
nsAppShell::Init()
{
  if (!sMsgId)
    sMsgId = RegisterWindowMessageW(L"nsAppShell:EventID");

#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_WIN7
  sTaskbarButtonCreatedMsg = ::RegisterWindowMessageW(L"TaskbarButtonCreated");
  NS_ASSERTION(sTaskbarButtonCreatedMsg, "Could not register taskbar button creation message");
#endif

  WNDCLASSW wc;
  HINSTANCE module = GetModuleHandle(NULL);

  const PRUnichar *const kWindowClass = L"nsAppShell:EventWindowClass";
  if (!GetClassInfoW(module, kWindowClass, &wc)) {
    wc.style         = 0;
    wc.lpfnWndProc   = EventWindowProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = module;
    wc.hIcon         = NULL;
    wc.hCursor       = NULL;
    wc.hbrBackground = (HBRUSH) NULL;
    wc.lpszMenuName  = (LPCWSTR) NULL;
    wc.lpszClassName = kWindowClass;
    RegisterClassW(&wc);
  }

  mEventWnd = CreateWindowW(kWindowClass, L"nsAppShell:EventWindow",
                           0, 0, 0, 10, 10, NULL, NULL, module, NULL);
  NS_ENSURE_STATE(mEventWnd);

  return nsBaseAppShell::Init();
}

void
nsAppShell::ScheduleNativeEventCallback()
{
  
  NS_ADDREF_THIS();
  ::PostMessage(mEventWnd, sMsgId, 0, reinterpret_cast<LPARAM>(this));
}

PRBool
nsAppShell::ProcessNextNativeEvent(PRBool mayWait)
{
  PRBool gotMessage = PR_FALSE;

  do {
    MSG msg;
    
    
    if (PeekKeyAndIMEMessage(&msg, NULL) ||
        ::PeekMessageW(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE) || 
        ::PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
      gotMessage = PR_TRUE;
      if (msg.message == WM_QUIT) {
        ::PostQuitMessage(msg.wParam);
        Exit();
      } else {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
      }
    } else if (mayWait) {
      
      ::WaitMessage();
    }
  } while (!gotMessage && mayWait);

  return gotMessage;
}
