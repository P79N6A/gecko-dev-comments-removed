






































#include "WindowsMessageLoop.h"
#include "SyncChannel.h"

#include "nsAutoPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsStringGlue.h"
#include "nsIXULAppInfo.h"

#include "mozilla/Mutex.h"

using mozilla::ipc::SyncChannel;
using mozilla::MutexAutoUnlock;

using namespace mozilla::ipc::windows;









































namespace {

UINT gEventLoopMessage =
    RegisterWindowMessage(L"SyncChannel Windows Message Loop Message");

const wchar_t kOldWndProcProp[] = L"MozillaIPCOldWndProc";


enum { WM_XP_THEMECHANGED = 0x031A };

PRUnichar gAppMessageWindowName[256] = { 0 };
PRInt32 gAppMessageWindowNameLength = 0;

nsTArray<HWND>* gNeuteredWindows = nsnull;

typedef nsTArray<nsAutoPtr<DeferredMessage> > DeferredMessageArray;
DeferredMessageArray* gDeferredMessages = nsnull;

HHOOK gDeferredGetMsgHook = NULL;
HHOOK gDeferredCallWndProcHook = NULL;

DWORD gUIThreadId = 0;
int gEventLoopDepth = 0;

LRESULT CALLBACK
DeferredMessageHook(int nCode,
                    WPARAM wParam,
                    LPARAM lParam)
{
  
  
  

  
  
  
  
  
  
  if (nCode >= 0 && gDeferredMessages && !SyncChannel::IsPumpingMessages()) {
    NS_ASSERTION(gDeferredGetMsgHook && gDeferredCallWndProcHook,
                 "These hooks must be set if we're being called!");
    NS_ASSERTION(gDeferredMessages->Length(), "No deferred messages?!");

    
    UnhookWindowsHookEx(gDeferredGetMsgHook);
    UnhookWindowsHookEx(gDeferredCallWndProcHook);
    gDeferredGetMsgHook = 0;
    gDeferredCallWndProcHook = 0;

    
    nsAutoPtr<DeferredMessageArray> messages(gDeferredMessages);
    gDeferredMessages = nsnull;

    
    PRUint32 count = messages->Length();
    for (PRUint32 index = 0; index < count; index++) {
      messages->ElementAt(index)->Run();
    }
  }

  
  return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT
ProcessOrDeferMessage(HWND hwnd,
                      UINT uMsg,
                      WPARAM wParam,
                      LPARAM lParam)
{
  DeferredMessage* deferred = nsnull;

  
  LRESULT res = 0;

  switch (uMsg) {
    
    
    case WM_ACTIVATE:
    case WM_ACTIVATEAPP:
    case WM_CAPTURECHANGED:
    case WM_CHILDACTIVATE:
    case WM_DESTROY:
    case WM_IME_NOTIFY:
    case WM_IME_SETCONTEXT:
    case WM_KILLFOCUS:
    case WM_NCDESTROY:
    case WM_PARENTNOTIFY:
    case WM_SETFOCUS:
    case WM_SHOWWINDOW: 
    case WM_XP_THEMECHANGED: {
      deferred = new DeferredSendMessage(hwnd, uMsg, wParam, lParam);
      break;
    }

    case WM_SETCURSOR: {
      
      res = TRUE;
      deferred = new DeferredSendMessage(hwnd, uMsg, wParam, lParam);
      break;
    }

    case WM_NCACTIVATE: {
      res = TRUE;
      deferred = new DeferredNCActivateMessage(hwnd, uMsg, wParam, lParam);
      break;
    }

    case WM_MOUSEACTIVATE: {
      res = MA_NOACTIVATE;
      deferred = new DeferredSendMessage(hwnd, uMsg, wParam, lParam);
      break;
    }

    
    
    
    case WM_ERASEBKGND: {
      UINT flags = RDW_INVALIDATE | RDW_ERASE | RDW_NOINTERNALPAINT |
                   RDW_NOFRAME | RDW_NOCHILDREN | RDW_ERASENOW;
      deferred = new DeferredRedrawMessage(hwnd, flags);
      break;
    }
    case WM_NCPAINT: {
      UINT flags = RDW_INVALIDATE | RDW_FRAME | RDW_NOINTERNALPAINT |
                   RDW_NOERASE | RDW_NOCHILDREN | RDW_ERASENOW;
      deferred = new DeferredRedrawMessage(hwnd, flags);
      break;
    }

    
    
    case WM_PAINT: {
      deferred = new DeferredUpdateMessage(hwnd);
      break;
    }

    
    case WM_SETTINGCHANGE: {
      deferred = new DeferredSettingChangeMessage(hwnd, uMsg, wParam, lParam);
      break;
    }

    
    case WM_WINDOWPOSCHANGED: 
    case WM_WINDOWPOSCHANGING: {
      UINT flags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE |
                   SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_DEFERERASE;
      deferred = new DeferredWindowPosMessage(hwnd, flags);
      break;
    }
    case WM_NCCALCSIZE: {
      UINT flags = SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE |
                   SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER |
                   SWP_DEFERERASE | SWP_NOSENDCHANGING;
      deferred = new DeferredWindowPosMessage(hwnd, flags);
      break;
    }

    
    case WM_GETICON:
    case WM_GETMINMAXINFO:
    case WM_GETTEXT:
    case WM_NCHITTEST:
    case WM_SETICON: 
    case WM_SYNCPAINT: {
      return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    
    default: {
#ifdef DEBUG
      nsCAutoString log("Received \"nonqueued\" message ");
      log.AppendInt(uMsg);
      log.AppendLiteral(" during a synchronous IPC message for window ");
      log.AppendInt((PRInt64)hwnd);

      wchar_t className[256] = { 0 };
      if (GetClassNameW(hwnd, className, sizeof(className) - 1) > 0) {
        log.AppendLiteral(" (\"");
        log.Append(NS_ConvertUTF16toUTF8((PRUnichar*)className));
        log.AppendLiteral("\")");
      }

      log.AppendLiteral(", sending it to DefWindowProc instead of the normal "
                        "window procedure.");
      NS_ERROR(log.get());
#endif
      return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
  }

  NS_ASSERTION(deferred, "Must have a message here!");

  
  if (!gDeferredMessages) {
    gDeferredMessages = new nsTArray<nsAutoPtr<DeferredMessage> >(20);
    NS_ASSERTION(gDeferredMessages, "Out of memory!");
  }

  
  gDeferredMessages->AppendElement(deferred);
  return res;
}

LRESULT CALLBACK
NeuteredWindowProc(HWND hwnd,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
  WNDPROC oldWndProc = (WNDPROC)GetProp(hwnd, kOldWndProcProp);
  if (!oldWndProc) {
    
    NS_ERROR("No old wndproc!");
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
  }

  
  
  return ProcessOrDeferMessage(hwnd, uMsg, wParam, lParam);
}

static bool
WindowIsMozillaWindow(HWND hWnd)
{
  if (!IsWindow(hWnd)) {
    NS_WARNING("Window has died!");
    return false;
  }

  PRUnichar buffer[256] = { 0 };
  int length = GetClassNameW(hWnd, (wchar_t*)buffer, sizeof(buffer) - 1);
  if (length <= 0) {
    NS_WARNING("Failed to get class name!");
    return false;
  }

  nsDependentString className(buffer, length);
  if (StringBeginsWith(className, NS_LITERAL_STRING("Mozilla")) ||
      StringBeginsWith(className, NS_LITERAL_STRING("Gecko")) ||
      className.EqualsLiteral("nsToolkitClass") ||
      className.EqualsLiteral("nsAppShell:EventWindowClass")) {
    return true;
  }

  
  
  
  if (gAppMessageWindowNameLength == 0) {
    nsCOMPtr<nsIXULAppInfo> appInfo =
      do_GetService("@mozilla.org/xre/app-info;1");
    if (appInfo) {
      nsCAutoString appName;
      if (NS_SUCCEEDED(appInfo->GetName(appName))) {
        appName.Append("MessageWindow");
        nsDependentString windowName(gAppMessageWindowName);
        CopyUTF8toUTF16(appName, windowName);
        gAppMessageWindowNameLength = windowName.Length();
      }
    }

    
    if (gAppMessageWindowNameLength == 0) {
      gAppMessageWindowNameLength = -1;
    }
  }

  if (gAppMessageWindowNameLength != -1 &&
      className.Equals(nsDependentString(gAppMessageWindowName,
                                         gAppMessageWindowNameLength))) {
    return true;
  }

  return false;
}

bool
NeuterWindowProcedure(HWND hWnd)
{
  if (!WindowIsMozillaWindow(hWnd)) {
    
    return false;
  }

  NS_ASSERTION(!GetProp(hWnd, kOldWndProcProp), "This should always be null!");

  
  
  
  SetLastError(ERROR_SUCCESS);

  LONG_PTR currentWndProc =
    SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)NeuteredWindowProc);
  if (!currentWndProc) {
    if (ERROR_SUCCESS == GetLastError()) {
      
      SetWindowLongPtr(hWnd, GWLP_WNDPROC, currentWndProc);
    }
    return false;
  }

  NS_ASSERTION(currentWndProc != (LONG_PTR)NeuteredWindowProc,
               "This shouldn't be possible!");

  if (!SetProp(hWnd, kOldWndProcProp, (HANDLE)currentWndProc)) {
    
    NS_WARNING("SetProp failed!");
    SetWindowLongPtr(hWnd, GWLP_WNDPROC, currentWndProc);
    RemoveProp(hWnd, kOldWndProcProp);
    return false;
  }

  return true;
}

void
RestoreWindowProcedure(HWND hWnd)
{
  NS_ASSERTION(WindowIsMozillaWindow(hWnd),
               "Not a mozilla window, this shouldn't be in our list!");

  LONG_PTR oldWndProc = (LONG_PTR)RemoveProp(hWnd, kOldWndProcProp);
  if (oldWndProc) {
    NS_ASSERTION(oldWndProc != (LONG_PTR)NeuteredWindowProc,
                 "This shouldn't be possible!");

    LONG_PTR currentWndProc =
      SetWindowLongPtr(hWnd, GWLP_WNDPROC, oldWndProc);
    NS_ASSERTION(currentWndProc == (LONG_PTR)NeuteredWindowProc,
                 "This should never be switched out from under us!");
  }
}

LRESULT CALLBACK
CallWindowProcedureHook(int nCode,
                        WPARAM wParam,
                        LPARAM lParam)
{
  if (nCode >= 0) {
    NS_ASSERTION(gNeuteredWindows, "This should never be null!");

    HWND hWnd = reinterpret_cast<CWPSTRUCT*>(lParam)->hwnd;

    if (!gNeuteredWindows->Contains(hWnd) && NeuterWindowProcedure(hWnd)) {
      if (!gNeuteredWindows->AppendElement(hWnd)) {
        NS_ERROR("Out of memory!");
        RestoreWindowProcedure(hWnd);
      }
    }
  }
  return CallNextHookEx(NULL, nCode, wParam, lParam);
}

inline void
AssertWindowIsNotNeutered(HWND hWnd)
{
#ifdef DEBUG
  
  LONG_PTR wndproc = GetWindowLongPtr(hWnd, GWLP_WNDPROC);
  NS_ASSERTION(wndproc != (LONG_PTR)NeuteredWindowProc, "Window is neutered!");
#endif
}

} 

void
SyncChannel::WaitForNotify()
{
  mMutex.AssertCurrentThreadOwns();

  NS_ASSERTION(gEventLoopDepth >= 0, "Event loop depth mismatch!");

  HHOOK windowHook = NULL;

  nsAutoTArray<HWND, 20> neuteredWindows;

  if (++gEventLoopDepth == 1) {
    NS_ASSERTION(!SyncChannel::IsPumpingMessages(),
                 "Shouldn't be pumping already!");
    SyncChannel::SetIsPumpingMessages(true);

    if (!gUIThreadId) {
      gUIThreadId = GetCurrentThreadId();
    }
    NS_ASSERTION(gUIThreadId, "ThreadId should not be 0!");
    NS_ASSERTION(gUIThreadId == GetCurrentThreadId(),
                 "Running on different threads!");

    NS_ASSERTION(!gNeuteredWindows, "Should only set this once!");
    gNeuteredWindows = &neuteredWindows;

    windowHook = SetWindowsHookEx(WH_CALLWNDPROC, CallWindowProcedureHook,
                                  NULL, gUIThreadId);
    NS_ASSERTION(windowHook, "Failed to set hook!");
  }

  {
    MutexAutoUnlock unlock(mMutex);

    while (1) {
      
      
      
      
      
      
      DWORD result = MsgWaitForMultipleObjects(0, NULL, FALSE, INFINITE,
                                               QS_ALLINPUT);
      if (result != WAIT_OBJECT_0) {
        NS_ERROR("Wait failed!");
        break;
      }

      
      
      
      
      
      
      
      bool haveSentMessagesPending =
        (HIWORD(GetQueueStatus(QS_SENDMESSAGE)) & QS_SENDMESSAGE) != 0;

      
      
      
      
      

      
      
      MSG msg = { 0 };
      if (PeekMessageW(&msg, (HWND)-1, gEventLoopMessage, gEventLoopMessage,
                       PM_REMOVE)) {
        break;
      }

      
      
      
      
      
      if (!PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE) &&
          !haveSentMessagesPending) {
        
        SwitchToThread();
      }
    }
  }

  NS_ASSERTION(gEventLoopDepth > 0, "Event loop depth mismatch!");

  if (--gEventLoopDepth == 0) {
    if (windowHook) {
      UnhookWindowsHookEx(windowHook);
    }

    NS_ASSERTION(gNeuteredWindows == &neuteredWindows, "Bad pointer!");
    gNeuteredWindows = nsnull;

    PRUint32 count = neuteredWindows.Length();
    for (PRUint32 index = 0; index < count; index++) {
      RestoreWindowProcedure(neuteredWindows[index]);
    }

    SyncChannel::SetIsPumpingMessages(false);

    
    
    
    
    if (gDeferredMessages &&
        !(gDeferredGetMsgHook && gDeferredCallWndProcHook)) {
      NS_ASSERTION(gDeferredMessages->Length(), "No deferred messages?!");

      gDeferredGetMsgHook = SetWindowsHookEx(WH_GETMESSAGE, DeferredMessageHook,
                                             NULL, gUIThreadId);
      gDeferredCallWndProcHook = SetWindowsHookEx(WH_CALLWNDPROC,
                                                  DeferredMessageHook, NULL,
                                                  gUIThreadId);
      NS_ASSERTION(gDeferredGetMsgHook && gDeferredCallWndProcHook,
                   "Failed to set hooks!");
    }
  }
}

void
SyncChannel::NotifyWorkerThread()
{
  mMutex.AssertCurrentThreadOwns();
  NS_ASSERTION(gUIThreadId, "This should have been set already!");
  if (!PostThreadMessage(gUIThreadId, gEventLoopMessage, 0, 0)) {
    NS_WARNING("Failed to post thread message!");
  }
}

void
DeferredSendMessage::Run()
{
  AssertWindowIsNotNeutered(hWnd);
  if (!IsWindow(hWnd)) {
    NS_ERROR("Invalid window!");
    return;
  }

  WNDPROC wndproc =
    reinterpret_cast<WNDPROC>(GetWindowLongPtr(hWnd, GWLP_WNDPROC));
  if (!wndproc) {
    NS_ERROR("Invalid window procedure!");
    return;
  }

  CallWindowProc(wndproc, hWnd, message, wParam, lParam);
}

void
DeferredRedrawMessage::Run()
{
  AssertWindowIsNotNeutered(hWnd);
  if (!IsWindow(hWnd)) {
    NS_ERROR("Invalid window!");
    return;
  }

#ifdef DEBUG
  BOOL ret =
#endif
  RedrawWindow(hWnd, NULL, NULL, flags);
  NS_ASSERTION(ret, "RedrawWindow failed!");
}

void
DeferredUpdateMessage::Run()
{
  AssertWindowIsNotNeutered(hWnd);
  if (!IsWindow(hWnd)) {
    NS_ERROR("Invalid window!");
    return;
  }

#ifdef DEBUG
  BOOL ret =
#endif
  UpdateWindow(hWnd);
  NS_ASSERTION(ret, "UpdateWindow failed!");
}

DeferredSettingChangeMessage::DeferredSettingChangeMessage(HWND aHWnd,
                                                           UINT aMessage,
                                                           WPARAM aWParam,
                                                           LPARAM aLParam)
: DeferredSendMessage(aHWnd, aMessage, aWParam, aLParam)
{
  NS_ASSERTION(aMessage == WM_SETTINGCHANGE, "Wrong message type!");
  if (aLParam) {
    lParamString = _wcsdup(reinterpret_cast<const wchar_t*>(aLParam));
    lParam = reinterpret_cast<LPARAM>(lParamString);
  }
  else {
    lParam = NULL;
  }
}

DeferredSettingChangeMessage::~DeferredSettingChangeMessage()
{
  if (lParamString) {
    free(lParamString);
  }
}

void
DeferredWindowPosMessage::Run()
{
  AssertWindowIsNotNeutered(hWnd);
  if (!IsWindow(hWnd)) {
    NS_ERROR("Invalid window!");
    return;
  }

#ifdef DEBUG
  BOOL ret = 
#endif
  SetWindowPos(hWnd, 0, 0, 0, 0, 0, flags);
  NS_ASSERTION(ret, "SetWindowPos failed!");
}

DeferredNCActivateMessage::DeferredNCActivateMessage(HWND aHWnd,
                                                     UINT aMessage,
                                                     WPARAM aWParam,
                                                     LPARAM aLParam)
: DeferredSendMessage(aHWnd, aMessage, aWParam, aLParam),
  region(NULL)
{
  NS_ASSERTION(aMessage == WM_NCACTIVATE, "Wrong message!");
  if (aLParam) {
    
    
    HRGN source = reinterpret_cast<HRGN>(aLParam);

    DWORD dataSize = GetRegionData(source, 0, NULL);
    if (!dataSize) {
      NS_ERROR("GetRegionData failed!");
      return;
    }

    nsAutoArrayPtr<char> buffer = new char[dataSize];
    NS_ASSERTION(buffer, "Out of memory!");

    RGNDATA* data = reinterpret_cast<RGNDATA*>(buffer.get());

    dataSize = GetRegionData(source, dataSize, data);
    if (!dataSize) {
      NS_ERROR("GetRegionData failed!");
      return;
    }

    HRGN tempRegion = ExtCreateRegion(NULL, dataSize, data);
    if (!tempRegion) {
      NS_ERROR("ExtCreateRegion failed!");
      return;
    }

    region = tempRegion;
    lParam = reinterpret_cast<LPARAM>(region);
  }
}

DeferredNCActivateMessage::~DeferredNCActivateMessage()
{
  if (region) {
    DeleteObject(region);
  }
}
