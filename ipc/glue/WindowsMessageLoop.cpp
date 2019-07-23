







































#include "WindowsMessageLoop.h"
#include "RPCChannel.h"

#include "nsAutoPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsStringGlue.h"
#include "nsIXULAppInfo.h"

#include "mozilla/Mutex.h"

using mozilla::ipc::SyncChannel;
using mozilla::ipc::RPCChannel;
using mozilla::MutexAutoUnlock;

using namespace mozilla::ipc::windows;
















































namespace {

UINT gEventLoopMessage =
    RegisterWindowMessage(L"SyncChannel Windows Message Loop Message");

UINT gOOPPSpinNativeLoopEvent =
    RegisterWindowMessage(L"SyncChannel Spin Inner Loop Message");

UINT gOOPPStopNativeLoopEvent =
    RegisterWindowMessage(L"SyncChannel Stop Inner Loop Message");

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

void
ScheduleDeferredMessageRun()
{
  if (gDeferredMessages &&
      !(gDeferredGetMsgHook && gDeferredCallWndProcHook)) {
    NS_ASSERTION(gDeferredMessages->Length(), "No deferred messages?!");

    gDeferredGetMsgHook = ::SetWindowsHookEx(WH_GETMESSAGE, DeferredMessageHook,
                                             NULL, gUIThreadId);
    gDeferredCallWndProcHook = ::SetWindowsHookEx(WH_CALLWNDPROC,
                                                  DeferredMessageHook, NULL,
                                                  gUIThreadId);
    NS_ASSERTION(gDeferredGetMsgHook && gDeferredCallWndProcHook,
                 "Failed to set hooks!");
  }
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
    case WM_CANCELMODE:
    case WM_CAPTURECHANGED:
    case WM_CHILDACTIVATE:
    case WM_DESTROY:
    case WM_ENABLE:
    case WM_IME_NOTIFY:
    case WM_IME_SETCONTEXT:
    case WM_KILLFOCUS:
    case WM_MOUSEWHEEL:
    case WM_NCDESTROY:
    case WM_PARENTNOTIFY:
    case WM_SETFOCUS:
    case WM_SYSCOMMAND:
    case WM_SHOWWINDOW: 
    case WM_XP_THEMECHANGED: {
      deferred = new DeferredSendMessage(hwnd, uMsg, wParam, lParam);
      break;
    }

    case WM_DEVICECHANGE:
    case WM_NCACTIVATE: 
    case WM_SETCURSOR: {
      
      res = TRUE;
      deferred = new DeferredSendMessage(hwnd, uMsg, wParam, lParam);
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

    
    case WM_WINDOWPOSCHANGED: {
      deferred = new DeferredWindowPosMessage(hwnd, lParam);
      break;
    }
    case WM_NCCALCSIZE: {
      deferred = new DeferredWindowPosMessage(hwnd, lParam, true, wParam);
      break;
    }

    case WM_COPYDATA: {
      deferred = new DeferredCopyDataMessage(hwnd, uMsg, wParam, lParam);
      res = TRUE;
      break;
    }

    case WM_STYLECHANGED: {
      deferred = new DeferredStyleChangeMessage(hwnd, wParam, lParam);
      break;
    }

    case WM_SETICON: {
      deferred = new DeferredSetIconMessage(hwnd, uMsg, wParam, lParam);
      break;
    }

    
    case WM_ENTERIDLE:
    case WM_GETICON:
    case WM_GETMINMAXINFO:
    case WM_GETTEXT:
    case WM_NCHITTEST:
    case WM_STYLECHANGING:
    case WM_SYNCPAINT: 
    case WM_WINDOWPOSCHANGING: {
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
WindowIsDeferredWindow(HWND hWnd)
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

  
  
  
  if (className.EqualsLiteral("ShockwaveFlashFullScreen") ||
      className.EqualsLiteral("AGFullScreenWinClass")) {
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
  if (!WindowIsDeferredWindow(hWnd)) {
    
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
  NS_ASSERTION(WindowIsDeferredWindow(hWnd),
               "Not a deferred window, this shouldn't be in our list!");

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

void
UnhookNeuteredWindows()
{
  if (!gNeuteredWindows)
    return;
  PRUint32 count = gNeuteredWindows->Length();
  for (PRUint32 index = 0; index < count; index++) {
    RestoreWindowProcedure(gNeuteredWindows->ElementAt(index));
  }
  gNeuteredWindows->Clear();
}

void
Init()
{
  
  
  if (!gUIThreadId) {
    gUIThreadId = GetCurrentThreadId();
  }
  NS_ASSERTION(gUIThreadId, "ThreadId should not be 0!");
  NS_ASSERTION(gUIThreadId == GetCurrentThreadId(),
               "Running on different threads!");
}





struct TimeoutData
{
  DWORD startTicks;
  DWORD targetTicks;
};

void
InitTimeoutData(TimeoutData* aData,
                int32 aTimeoutMs)
{
  aData->startTicks = GetTickCount();
  if (!aData->startTicks) {
    
    aData->startTicks++;
  }
  aData->targetTicks = aData->startTicks + aTimeoutMs;
}


bool
TimeoutHasExpired(const TimeoutData& aData)
{
  if (!aData.startTicks) {
    return false;
  }

  DWORD now = GetTickCount();

  if (aData.targetTicks < aData.startTicks) {
    
    return now < aData.startTicks && now >= aData.targetTicks;
  }
  return now >= aData.targetTicks;
}

} 

bool
RPCChannel::SpinInternalEventLoop()
{
  
  
  do {
    MSG msg = { 0 };

    
    {
      MutexAutoLock lock(mMutex);
      if (!Connected()) {
        RPCChannel::ExitModalLoop();
        return false;
      }
    }

    if (!RPCChannel::IsSpinLoopActive()) {
      return false;
    }

    
    
    
    
    if (PeekMessageW(&msg, (HWND)-1, gOOPPStopNativeLoopEvent,
                     gOOPPStopNativeLoopEvent, PM_REMOVE)) {
      RPCChannel::ExitModalLoop();
      return false;
    }

    
    
    
    if (PeekMessageW(&msg, (HWND)-1, gEventLoopMessage, gEventLoopMessage,
                     PM_REMOVE)) {
      return true;
    }

    
    if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
      if (msg.message == gOOPPStopNativeLoopEvent) {
        RPCChannel::ExitModalLoop();
        return false;
      }
      else if (msg.message == gOOPPSpinNativeLoopEvent) {
        
        RPCChannel::EnterModalLoop();
        continue;
      }
      else if (msg.message == gEventLoopMessage) {
        return true;
      }

      
      
      if (msg.message == WM_QUIT) {
          NS_ERROR("WM_QUIT received in SpinInternalEventLoop!");
      } else {
          TranslateMessage(&msg);
          DispatchMessageW(&msg);
      }
    } else {
      
      WaitMessage();
    }
  } while (true);
}

bool
RPCChannel::IsMessagePending()
{
  MSG msg = { 0 };
  if (PeekMessageW(&msg, (HWND)-1, gEventLoopMessage, gEventLoopMessage,
                   PM_REMOVE)) {
    return true;
  }
  return false;
}

bool
SyncChannel::WaitForNotify()
{
  mMutex.AssertCurrentThreadOwns();

  
  Init();

  MutexAutoUnlock unlock(mMutex);

  bool retval = true;

  if (++gEventLoopDepth == 1) {
    NS_ASSERTION(!gNeuteredWindows, "Should only set this once!");
    gNeuteredWindows = new nsAutoTArray<HWND, 20>();
    NS_ASSERTION(gNeuteredWindows, "Out of memory!");
  }

  UINT_PTR timerId = NULL;
  TimeoutData timeoutData = { 0 };

  if (mTimeoutMs != kNoTimeout) {
    InitTimeoutData(&timeoutData, mTimeoutMs);

    
    
    timerId = SetTimer(NULL, 0, mTimeoutMs, NULL);
    NS_ASSERTION(timerId, "SetTimer failed!");
  }

  
  NS_ASSERTION(!SyncChannel::IsPumpingMessages(),
               "Shouldn't be pumping already!");
  SyncChannel::SetIsPumpingMessages(true);
  HHOOK windowHook = SetWindowsHookEx(WH_CALLWNDPROC, CallWindowProcedureHook,
                                      NULL, gUIThreadId);
  NS_ASSERTION(windowHook, "Failed to set hook!");

  {
    while (1) {
      MSG msg = { 0 };
      
      {
        MutexAutoLock lock(mMutex);
        if (!Connected()) {
          break;
        }
      }

      
      
      
      
      
      
      DWORD result = MsgWaitForMultipleObjects(0, NULL, FALSE, INFINITE,
                                               QS_ALLINPUT);
      if (result != WAIT_OBJECT_0) {
        NS_ERROR("Wait failed!");
        break;
      }

      if (TimeoutHasExpired(timeoutData)) {
        
        retval = false;
        break;
      }

      
      
      
      
      
      
      
      bool haveSentMessagesPending =
        (HIWORD(GetQueueStatus(QS_SENDMESSAGE)) & QS_SENDMESSAGE) != 0;

      
      
      
      
      

      
      
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

  
  UnhookWindowsHookEx(windowHook);

  
  
  UnhookNeuteredWindows();

  if (--gEventLoopDepth == 0) {
    NS_ASSERTION(gNeuteredWindows, "Bad pointer!");
    delete gNeuteredWindows;
    gNeuteredWindows = NULL;
  }

  
  
  
  
  ScheduleDeferredMessageRun();

  if (timerId) {
    KillTimer(NULL, timerId);
  }

  SyncChannel::SetIsPumpingMessages(false);

  return retval;
}

bool
RPCChannel::WaitForNotify()
{
  mMutex.AssertCurrentThreadOwns();

  if (!StackDepth() && !mBlockedOnParent) {
    
    NS_RUNTIMEABORT("StackDepth() is 0 in call to RPCChannel::WaitForNotify!");
  }

  
  Init();

  MutexAutoUnlock unlock(mMutex);

  bool retval = true;

  
  
  
  
  
  
  
  
  
  if (RPCChannel::IsSpinLoopActive()) {
    SpinInternalEventLoop();
    return true; 
  }

  if (++gEventLoopDepth == 1) {
    NS_ASSERTION(!gNeuteredWindows, "Should only set this once!");
    gNeuteredWindows = new nsAutoTArray<HWND, 20>();
    NS_ASSERTION(gNeuteredWindows, "Out of memory!");
  }

  UINT_PTR timerId = NULL;
  TimeoutData timeoutData = { 0 };

  if (mTimeoutMs != kNoTimeout) {
    InitTimeoutData(&timeoutData, mTimeoutMs);

    
    
    timerId = SetTimer(NULL, 0, mTimeoutMs, NULL);
    NS_ASSERTION(timerId, "SetTimer failed!");
  }

  
  NS_ASSERTION(!SyncChannel::IsPumpingMessages(),
               "Shouldn't be pumping already!");
  SyncChannel::SetIsPumpingMessages(true);
  HHOOK windowHook = SetWindowsHookEx(WH_CALLWNDPROC, CallWindowProcedureHook,
                                      NULL, gUIThreadId);
  NS_ASSERTION(windowHook, "Failed to set hook!");

  {
    while (1) {
      MSG msg = { 0 };

      
      {
        MutexAutoLock lock(mMutex);
        if (!Connected()) {
          break;
        }
      }

      DWORD result = MsgWaitForMultipleObjects(0, NULL, FALSE, INFINITE,
                                               QS_ALLINPUT);
      if (result != WAIT_OBJECT_0) {
        NS_ERROR("Wait failed!");
        break;
      }

      if (TimeoutHasExpired(timeoutData)) {
        
        retval = false;
        break;
      }

      
      bool haveSentMessagesPending =
        (HIWORD(GetQueueStatus(QS_SENDMESSAGE)) & QS_SENDMESSAGE) != 0;

      
      
      
      
      
      
      if (PeekMessageW(&msg, (HWND)-1, gOOPPSpinNativeLoopEvent,
                       gOOPPSpinNativeLoopEvent, PM_REMOVE)) {
        
        UnhookWindowsHookEx(windowHook);
        windowHook = NULL;

        if (timerId) {
          KillTimer(NULL, timerId);
          timerId = NULL;
        }

        
        SyncChannel::SetIsPumpingMessages(false);

        
        
        UnhookNeuteredWindows();

        
        
        
        ScheduleDeferredMessageRun();
        
        
        
        
        
        RPCChannel::EnterModalLoop();
        SpinInternalEventLoop();
        return true; 
      }

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

  
  UnhookWindowsHookEx(windowHook);

  
  
  UnhookNeuteredWindows();

  if (--gEventLoopDepth == 0) {
    NS_ASSERTION(gNeuteredWindows, "Bad pointer!");
    delete gNeuteredWindows;
    gNeuteredWindows = NULL;
  }

  
  
  
  
  ScheduleDeferredMessageRun();

  if (timerId) {
    KillTimer(NULL, timerId);
  }

  SyncChannel::SetIsPumpingMessages(false);

  return retval;
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
    lParamString = NULL;
    lParam = NULL;
  }
}

DeferredSettingChangeMessage::~DeferredSettingChangeMessage()
{
  free(lParamString);
}

DeferredWindowPosMessage::DeferredWindowPosMessage(HWND aHWnd,
                                                   LPARAM aLParam,
                                                   bool aForCalcSize,
                                                   WPARAM aWParam)
{
  if (aForCalcSize) {
    if (aWParam) {
      NCCALCSIZE_PARAMS* arg = reinterpret_cast<NCCALCSIZE_PARAMS*>(aLParam);
      memcpy(&windowPos, arg->lppos, sizeof(windowPos));

      NS_ASSERTION(aHWnd == windowPos.hwnd, "Mismatched hwnds!");
    }
    else {
      RECT* arg = reinterpret_cast<RECT*>(aLParam);
      windowPos.hwnd = aHWnd;
      windowPos.hwndInsertAfter = NULL;
      windowPos.x = arg->left;
      windowPos.y = arg->top;
      windowPos.cx = arg->right - arg->left;
      windowPos.cy = arg->bottom - arg->top;

      NS_ASSERTION(arg->right >= arg->left && arg->bottom >= arg->top,
                   "Negative width or height!");
    }
    windowPos.flags = SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOOWNERZORDER |
                      SWP_NOZORDER | SWP_DEFERERASE | SWP_NOSENDCHANGING;
  }
  else {
    
    WINDOWPOS* arg = reinterpret_cast<WINDOWPOS*>(aLParam);
    memcpy(&windowPos, arg, sizeof(windowPos));

    NS_ASSERTION(aHWnd == windowPos.hwnd, "Mismatched hwnds!");

    
    
    UINT mask = SWP_ASYNCWINDOWPOS | SWP_DEFERERASE | SWP_DRAWFRAME |
                SWP_FRAMECHANGED | SWP_HIDEWINDOW | SWP_NOACTIVATE |
                SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOREDRAW |
                SWP_NOREPOSITION | SWP_NOSENDCHANGING | SWP_NOSIZE |
                SWP_NOZORDER | SWP_SHOWWINDOW;
    windowPos.flags &= mask;
  }
}

void
DeferredWindowPosMessage::Run()
{
  AssertWindowIsNotNeutered(windowPos.hwnd);
  if (!IsWindow(windowPos.hwnd)) {
    NS_ERROR("Invalid window!");
    return;
  }

  if (!IsWindow(windowPos.hwndInsertAfter)) {
    NS_WARNING("ZOrder change cannot be honored");
    windowPos.hwndInsertAfter = 0;
    windowPos.flags |= SWP_NOZORDER;
  }

#ifdef DEBUG
  BOOL ret = 
#endif
  SetWindowPos(windowPos.hwnd, windowPos.hwndInsertAfter, windowPos.x,
               windowPos.y, windowPos.cx, windowPos.cy, windowPos.flags);
  NS_ASSERTION(ret, "SetWindowPos failed!");
}

DeferredCopyDataMessage::DeferredCopyDataMessage(HWND aHWnd,
                                                 UINT aMessage,
                                                 WPARAM aWParam,
                                                 LPARAM aLParam)
: DeferredSendMessage(aHWnd, aMessage, aWParam, aLParam)
{
  NS_ASSERTION(IsWindow(reinterpret_cast<HWND>(aWParam)), "Bad window!");

  COPYDATASTRUCT* source = reinterpret_cast<COPYDATASTRUCT*>(aLParam);
  NS_ASSERTION(source, "Should never be null!");

  copyData.dwData = source->dwData;
  copyData.cbData = source->cbData;

  if (source->cbData) {
    copyData.lpData = malloc(source->cbData);
    if (copyData.lpData) {
      memcpy(copyData.lpData, source->lpData, source->cbData);
    }
    else {
      NS_ERROR("Out of memory?!");
      copyData.cbData = 0;
    }
  }
  else {
    copyData.lpData = NULL;
  }

  lParam = reinterpret_cast<LPARAM>(&copyData);
}

DeferredCopyDataMessage::~DeferredCopyDataMessage()
{
  free(copyData.lpData);
}

DeferredStyleChangeMessage::DeferredStyleChangeMessage(HWND aHWnd,
                                                       WPARAM aWParam,
                                                       LPARAM aLParam)
: hWnd(aHWnd)
{
  index = static_cast<int>(aWParam);
  style = reinterpret_cast<STYLESTRUCT*>(aLParam)->styleNew;
}

void
DeferredStyleChangeMessage::Run()
{
  SetWindowLongPtr(hWnd, index, style);
}

DeferredSetIconMessage::DeferredSetIconMessage(HWND aHWnd,
                                               UINT aMessage,
                                               WPARAM aWParam,
                                               LPARAM aLParam)
: DeferredSendMessage(aHWnd, aMessage, aWParam, aLParam)
{
  NS_ASSERTION(aMessage == WM_SETICON, "Wrong message type!");
}

void
DeferredSetIconMessage::Run()
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

  HICON hOld = reinterpret_cast<HICON>(
    CallWindowProc(wndproc, hWnd, message, wParam, lParam));
  if (hOld) {
    DestroyIcon(hOld);
  }
}
