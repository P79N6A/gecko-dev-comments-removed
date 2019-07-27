






#include "mozilla/DebugOnly.h"

#include "WindowsMessageLoop.h"
#include "MessageChannel.h"

#include "nsAutoPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "nsIXULAppInfo.h"
#include "WinUtils.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/PaintTracker.h"

using namespace mozilla;
using namespace mozilla::ipc;
using namespace mozilla::ipc::windows;
















































#if defined(ACCESSIBILITY)

extern const wchar_t* kPropNameTabContent;
#endif


namespace mozilla {
namespace widget {
extern UINT sAppShellGeckoMsgId;
}
}

namespace {

const wchar_t kOldWndProcProp[] = L"MozillaIPCOldWndProc";
const wchar_t k3rdPartyWindowProp[] = L"Mozilla3rdPartyWindow";


enum { WM_XP_THEMECHANGED = 0x031A };

char16_t gAppMessageWindowName[256] = { 0 };
int32_t gAppMessageWindowNameLength = 0;

nsTArray<HWND>* gNeuteredWindows = nullptr;

typedef nsTArray<nsAutoPtr<DeferredMessage> > DeferredMessageArray;
DeferredMessageArray* gDeferredMessages = nullptr;

HHOOK gDeferredGetMsgHook = nullptr;
HHOOK gDeferredCallWndProcHook = nullptr;

DWORD gUIThreadId = 0;
HWND gCOMWindow = 0;


HWINEVENTHOOK gWinEventHook = nullptr;
const wchar_t kCOMWindowClassName[] = L"OleMainThreadWndClass";


#define MOZOBJID_UIAROOT -25

HWND
FindCOMWindow()
{
  MOZ_ASSERT(gUIThreadId);

  HWND last = 0;
  while ((last = FindWindowExW(HWND_MESSAGE, last, kCOMWindowClassName, NULL))) {
    if (GetWindowThreadProcessId(last, NULL) == gUIThreadId) {
      return last;
    }
  }

  return (HWND)0;
}

void CALLBACK
WinEventHook(HWINEVENTHOOK aWinEventHook, DWORD aEvent, HWND aHwnd,
             LONG aIdObject, LONG aIdChild, DWORD aEventThread,
             DWORD aMsEventTime)
{
  MOZ_ASSERT(aWinEventHook == gWinEventHook);
  MOZ_ASSERT(gUIThreadId == aEventThread);
  switch (aEvent) {
    case EVENT_OBJECT_CREATE: {
      if (aIdObject != OBJID_WINDOW || aIdChild != CHILDID_SELF) {
        
        return;
      }
      wchar_t classBuf[256] = {0};
      int result = ::GetClassNameW(aHwnd, classBuf,
                                   MOZ_ARRAY_LENGTH(classBuf));
      if (result != (MOZ_ARRAY_LENGTH(kCOMWindowClassName) - 1) ||
          wcsncmp(kCOMWindowClassName, classBuf, result)) {
        
        return;
      }
      MOZ_ASSERT(FindCOMWindow() == aHwnd);
      gCOMWindow = aHwnd;
      break;
    }
    case EVENT_OBJECT_DESTROY: {
      if (aHwnd == gCOMWindow && aIdObject == OBJID_WINDOW) {
        MOZ_ASSERT(aIdChild == CHILDID_SELF);
        gCOMWindow = 0;
      }
      break;
    }
    default: {
      return;
    }
  }
}

LRESULT CALLBACK
DeferredMessageHook(int nCode,
                    WPARAM wParam,
                    LPARAM lParam)
{
  
  
  

  
  
  
  
  
  
  if (nCode >= 0 && gDeferredMessages && !MessageChannel::IsPumpingMessages()) {
    NS_ASSERTION(gDeferredGetMsgHook && gDeferredCallWndProcHook,
                 "These hooks must be set if we're being called!");
    NS_ASSERTION(gDeferredMessages->Length(), "No deferred messages?!");

    
    UnhookWindowsHookEx(gDeferredGetMsgHook);
    UnhookWindowsHookEx(gDeferredCallWndProcHook);
    gDeferredGetMsgHook = 0;
    gDeferredCallWndProcHook = 0;

    
    nsAutoPtr<DeferredMessageArray> messages(gDeferredMessages);
    gDeferredMessages = nullptr;

    
    uint32_t count = messages->Length();
    for (uint32_t index = 0; index < count; index++) {
      messages->ElementAt(index)->Run();
    }
  }

  
  return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void
ScheduleDeferredMessageRun()
{
  if (gDeferredMessages &&
      !(gDeferredGetMsgHook && gDeferredCallWndProcHook)) {
    NS_ASSERTION(gDeferredMessages->Length(), "No deferred messages?!");

    gDeferredGetMsgHook = ::SetWindowsHookEx(WH_GETMESSAGE, DeferredMessageHook,
                                             nullptr, gUIThreadId);
    gDeferredCallWndProcHook = ::SetWindowsHookEx(WH_CALLWNDPROC,
                                                  DeferredMessageHook, nullptr,
                                                  gUIThreadId);
    NS_ASSERTION(gDeferredGetMsgHook && gDeferredCallWndProcHook,
                 "Failed to set hooks!");
  }
}

static void
DumpNeuteredMessage(HWND hwnd, UINT uMsg)
{
#ifdef DEBUG
  nsAutoCString log("Received \"nonqueued\" ");
  
  if (uMsg < WM_USER) {
    int idx = 0;
    while (mozilla::widget::gAllEvents[idx].mId != (long)uMsg &&
           mozilla::widget::gAllEvents[idx].mStr != nullptr) {
      idx++;
    }
    if (mozilla::widget::gAllEvents[idx].mStr) {
      log.AppendPrintf("ui message \"%s\"", mozilla::widget::gAllEvents[idx].mStr);
    } else {
      log.AppendPrintf("ui message (0x%X)", uMsg);
    }
  } else if (uMsg >= WM_USER && uMsg < WM_APP) {
    log.AppendPrintf("WM_USER message (0x%X)", uMsg);
  } else if (uMsg >= WM_APP && uMsg < 0xC000) {
    log.AppendPrintf("WM_APP message (0x%X)", uMsg);
  } else if (uMsg >= 0xC000 && uMsg < 0x10000) {
    log.AppendPrintf("registered windows message (0x%X)", uMsg);
  } else {
    log.AppendPrintf("system message (0x%X)", uMsg);
  }

  log.AppendLiteral(" during a synchronous IPC message for window ");
  log.AppendPrintf("0x%X", hwnd);

  wchar_t className[256] = { 0 };
  if (GetClassNameW(hwnd, className, sizeof(className) - 1) > 0) {
    log.AppendLiteral(" (\"");
    log.Append(NS_ConvertUTF16toUTF8((char16_t*)className));
    log.AppendLiteral("\")");
  }

  log.AppendLiteral(", sending it to DefWindowProc instead of the normal "
                    "window procedure.");
  NS_ERROR(log.get());
#endif
}

LRESULT
ProcessOrDeferMessage(HWND hwnd,
                      UINT uMsg,
                      WPARAM wParam,
                      LPARAM lParam)
{
  DeferredMessage* deferred = nullptr;

  
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
    case WM_DISPLAYCHANGE:
    case WM_SHOWWINDOW: 
    case WM_XP_THEMECHANGED: {
      deferred = new DeferredSendMessage(hwnd, uMsg, wParam, lParam);
      break;
    }

    case WM_DEVICECHANGE:
    case WM_POWERBROADCAST:
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
    case WM_NCPAINT: 
    case WM_GETMINMAXINFO:
    case WM_GETTEXT:
    case WM_NCHITTEST:
    case WM_STYLECHANGING:  
    case WM_WINDOWPOSCHANGING:
    case WM_GETTEXTLENGTH: {
      return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    
    
    
    case WM_SYNCPAINT:
      return 0;

    
    
    case WM_APP-1:
      return 0;

    
    
#if defined(ACCESSIBILITY)
   case WM_GETOBJECT: {
      if (!::GetPropW(hwnd, k3rdPartyWindowProp)) {
        DWORD objId = static_cast<DWORD>(lParam);
        if ((objId == OBJID_CLIENT || objId == MOZOBJID_UIAROOT)) {
          WNDPROC oldWndProc = (WNDPROC)GetProp(hwnd, kOldWndProcProp);
          if (oldWndProc) {
            return CallWindowProcW(oldWndProc, hwnd, uMsg, wParam, lParam);
          }
        }
      }
      return DefWindowProc(hwnd, uMsg, wParam, lParam);
   }
#endif 

    default: {
      
      
      if (uMsg && uMsg == mozilla::widget::sAppShellGeckoMsgId) {
        
        deferred = new DeferredSendMessage(hwnd, uMsg, wParam, lParam);
      }
    }
  }

  
  
  if (!deferred) {
    DumpNeuteredMessage(hwnd, uMsg);
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
  }

  
  if (!gDeferredMessages) {
    gDeferredMessages = new nsTArray<nsAutoPtr<DeferredMessage> >(20);
    NS_ASSERTION(gDeferredMessages, "Out of memory!");
  }

  
  gDeferredMessages->AppendElement(deferred);
  return res;
}

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

namespace {

static bool
WindowIsDeferredWindow(HWND hWnd)
{
  if (!IsWindow(hWnd)) {
    NS_WARNING("Window has died!");
    return false;
  }

  char16_t buffer[256] = { 0 };
  int length = GetClassNameW(hWnd, (wchar_t*)buffer, sizeof(buffer) - 1);
  if (length <= 0) {
    NS_WARNING("Failed to get class name!");
    return false;
  }

#if defined(ACCESSIBILITY)
  
  
  if (::GetPropW(hWnd, kPropNameTabContent)) {
    return false;
  }
#endif

  
  nsDependentString className(buffer, length);
  if (StringBeginsWith(className, NS_LITERAL_STRING("Mozilla")) ||
      StringBeginsWith(className, NS_LITERAL_STRING("Gecko")) ||
      className.EqualsLiteral("nsToolkitClass") ||
      className.EqualsLiteral("nsAppShell:EventWindowClass")) {
    return true;
  }

  
  
  
  
  if (className.EqualsLiteral("ShockwaveFlashFullScreen") ||
      className.EqualsLiteral("QTNSHIDDEN") ||
      className.EqualsLiteral("AGFullScreenWinClass")) {
    SetPropW(hWnd, k3rdPartyWindowProp, (HANDLE)1);
    return true;
  }

  
  
  
  if (className.EqualsLiteral("__geplugin_bridge_window__")) {
    SetPropW(hWnd, k3rdPartyWindowProp, (HANDLE)1);
    return true;
  }

  
  
  
  if (gAppMessageWindowNameLength == 0) {
    nsCOMPtr<nsIXULAppInfo> appInfo =
      do_GetService("@mozilla.org/xre/app-info;1");
    if (appInfo) {
      nsAutoCString appName;
      if (NS_SUCCEEDED(appInfo->GetName(appName))) {
        appName.AppendLiteral("MessageWindow");
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
    RemovePropW(hWnd, kOldWndProcProp);
    RemovePropW(hWnd, k3rdPartyWindowProp);
    return false;
  }

  return true;
}

void
RestoreWindowProcedure(HWND hWnd)
{
  NS_ASSERTION(WindowIsDeferredWindow(hWnd),
               "Not a deferred window, this shouldn't be in our list!");
  LONG_PTR oldWndProc = (LONG_PTR)GetProp(hWnd, kOldWndProcProp);
  if (oldWndProc) {
    NS_ASSERTION(oldWndProc != (LONG_PTR)NeuteredWindowProc,
                 "This shouldn't be possible!");

    DebugOnly<LONG_PTR> currentWndProc =
      SetWindowLongPtr(hWnd, GWLP_WNDPROC, oldWndProc);
    NS_ASSERTION(currentWndProc == (LONG_PTR)NeuteredWindowProc,
                 "This should never be switched out from under us!");
  }
  RemovePropW(hWnd, kOldWndProcProp);
  RemovePropW(hWnd, k3rdPartyWindowProp);
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
  return CallNextHookEx(nullptr, nCode, wParam, lParam);
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
  uint32_t count = gNeuteredWindows->Length();
  for (uint32_t index = 0; index < count; index++) {
    RestoreWindowProcedure(gNeuteredWindows->ElementAt(index));
  }
  gNeuteredWindows->Clear();
}





struct TimeoutData
{
  DWORD startTicks;
  DWORD targetTicks;
};

void
InitTimeoutData(TimeoutData* aData,
                int32_t aTimeoutMs)
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

namespace mozilla {
namespace ipc {
namespace windows {

void
InitUIThread()
{
  
  
  if (!gUIThreadId) {
    gUIThreadId = GetCurrentThreadId();
  }

  MOZ_ASSERT(gUIThreadId);
  MOZ_ASSERT(gUIThreadId == GetCurrentThreadId(),
             "Called InitUIThread multiple times on different threads!");

  if (!gWinEventHook) {
    gWinEventHook = SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_DESTROY,
                                    NULL, &WinEventHook, GetCurrentProcessId(),
                                    gUIThreadId, WINEVENT_OUTOFCONTEXT);

    
    
    gCOMWindow = FindCOMWindow();
  }
  MOZ_ASSERT(gWinEventHook);
}

} 
} 
} 


MessageChannel::SyncStackFrame::SyncStackFrame(MessageChannel* channel, bool interrupt)
  : mInterrupt(interrupt)
  , mSpinNestedEvents(false)
  , mListenerNotified(false)
  , mChannel(channel)
  , mPrev(mChannel->mTopFrame)
  , mStaticPrev(sStaticTopFrame)
{
  
  
  if (!(mChannel->GetChannelFlags() & REQUIRE_DEFERRED_MESSAGE_PROTECTION)) {
    return;
  }

  mChannel->mTopFrame = this;
  sStaticTopFrame = this;

  if (!mStaticPrev) {
    NS_ASSERTION(!gNeuteredWindows, "Should only set this once!");
    gNeuteredWindows = new nsAutoTArray<HWND, 20>();
    NS_ASSERTION(gNeuteredWindows, "Out of memory!");
  }
}

MessageChannel::SyncStackFrame::~SyncStackFrame()
{
  if (!(mChannel->GetChannelFlags() & REQUIRE_DEFERRED_MESSAGE_PROTECTION)) {
    return;
  }

  NS_ASSERTION(this == mChannel->mTopFrame,
               "Mismatched interrupt stack frames");
  NS_ASSERTION(this == sStaticTopFrame,
               "Mismatched static Interrupt stack frames");

  mChannel->mTopFrame = mPrev;
  sStaticTopFrame = mStaticPrev;

  if (!mStaticPrev) {
    NS_ASSERTION(gNeuteredWindows, "Bad pointer!");
    delete gNeuteredWindows;
    gNeuteredWindows = nullptr;
  }
}

MessageChannel::SyncStackFrame* MessageChannel::sStaticTopFrame;





void 
MessageChannel::NotifyGeckoEventDispatch()
{
  
  if (!sStaticTopFrame || sStaticTopFrame->mListenerNotified)
    return;

  sStaticTopFrame->mListenerNotified = true;
  MessageChannel* channel = static_cast<MessageChannel*>(sStaticTopFrame->mChannel);
  channel->Listener()->ProcessRemoteNativeEventsInInterruptCall();
}



void
MessageChannel::ProcessNativeEventsInInterruptCall()
{
  NS_ASSERTION(GetCurrentThreadId() == gUIThreadId,
               "Shouldn't be on a non-main thread in here!");
  if (!mTopFrame) {
    NS_ERROR("Spin logic error: no Interrupt frame");
    return;
  }

  mTopFrame->mSpinNestedEvents = true;
}







void
MessageChannel::SpinInternalEventLoop()
{
  if (mozilla::PaintTracker::IsPainting()) {
    NS_RUNTIMEABORT("Don't spin an event loop while painting.");
  }

  NS_ASSERTION(mTopFrame && mTopFrame->mSpinNestedEvents,
               "Spinning incorrectly");

  
  
  
  
  

  do {
    MSG msg = { 0 };

    
    {
      MonitorAutoLock lock(*mMonitor);
      if (!Connected()) {
        return;
      }
    }

    
    if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
      
      
      if (msg.message == WM_QUIT) {
          NS_ERROR("WM_QUIT received in SpinInternalEventLoop!");
      } else {
          TranslateMessage(&msg);
          ::DispatchMessageW(&msg);
          return;
      }
    }

    
    
    
    

    
    DWORD result = MsgWaitForMultipleObjects(1, &mEvent, FALSE, INFINITE,
                                             QS_ALLINPUT);
    if (result == WAIT_OBJECT_0) {
      
      return;
    }
  } while (true);
}

static inline bool
IsTimeoutExpired(PRIntervalTime aStart, PRIntervalTime aTimeout)
{
  return (aTimeout != PR_INTERVAL_NO_TIMEOUT) &&
    (aTimeout <= (PR_IntervalNow() - aStart));
}

bool
MessageChannel::WaitForSyncNotify()
{
  mMonitor->AssertCurrentThreadOwns();

  MOZ_ASSERT(gUIThreadId, "InitUIThread was not called!");

  
  
  if (!(mFlags & REQUIRE_DEFERRED_MESSAGE_PROTECTION)) {
    PRIntervalTime timeout = (kNoTimeout == mTimeoutMs) ?
                             PR_INTERVAL_NO_TIMEOUT :
                             PR_MillisecondsToInterval(mTimeoutMs);
    PRIntervalTime waitStart = 0;

    if (timeout != PR_INTERVAL_NO_TIMEOUT) {
      waitStart = PR_IntervalNow();
    }

    MOZ_ASSERT(!mIsSyncWaitingOnNonMainThread);
    mIsSyncWaitingOnNonMainThread = true;

    mMonitor->Wait(timeout);

    MOZ_ASSERT(mIsSyncWaitingOnNonMainThread);
    mIsSyncWaitingOnNonMainThread = false;

    
    
    return WaitResponse(timeout == PR_INTERVAL_NO_TIMEOUT ?
                        false : IsTimeoutExpired(waitStart, timeout));
  }

  NS_ASSERTION(mFlags & REQUIRE_DEFERRED_MESSAGE_PROTECTION,
               "Shouldn't be here for channels that don't use message deferral!");
  NS_ASSERTION(mTopFrame && !mTopFrame->mInterrupt,
               "Top frame is not a sync frame!");

  MonitorAutoUnlock unlock(*mMonitor);

  bool timedout = false;

  UINT_PTR timerId = 0;
  TimeoutData timeoutData = { 0 };

  if (mTimeoutMs != kNoTimeout) {
    InitTimeoutData(&timeoutData, mTimeoutMs);

    
    
    timerId = SetTimer(nullptr, 0, mTimeoutMs, nullptr);
    NS_ASSERTION(timerId, "SetTimer failed!");
  }

  
  NS_ASSERTION(!MessageChannel::IsPumpingMessages(),
               "Shouldn't be pumping already!");

  MessageChannel::SetIsPumpingMessages(true);
  HHOOK windowHook = SetWindowsHookEx(WH_CALLWNDPROC, CallWindowProcedureHook,
                                      nullptr, gUIThreadId);
  NS_ASSERTION(windowHook, "Failed to set hook!");

  {
    while (1) {
      MSG msg = { 0 };
      
      {
        MonitorAutoLock lock(*mMonitor);
        if (!Connected()) {
          break;
        }
      }

      
      
      
      
      
      
      DWORD result = MsgWaitForMultipleObjects(1, &mEvent, FALSE, INFINITE,
                                               QS_ALLINPUT);
      if (result == WAIT_OBJECT_0) {
        
        ResetEvent(mEvent);
        break;
      } else
      if (result != (WAIT_OBJECT_0 + 1)) {
        NS_ERROR("Wait failed!");
        break;
      }

      if (TimeoutHasExpired(timeoutData)) {
        
        timedout = true;
        break;
      }

      
      
      
      
      
      
      
      bool haveSentMessagesPending =
        (HIWORD(GetQueueStatus(QS_SENDMESSAGE)) & QS_SENDMESSAGE) != 0;

      
      
      
      
      

      
      
      if (gCOMWindow) {
        if (PeekMessageW(&msg, gCOMWindow, 0, 0, PM_REMOVE)) {
          TranslateMessage(&msg);
          ::DispatchMessageW(&msg);
        }
      }

      
      
      
      
      
      if (!PeekMessageW(&msg, nullptr, 0, 0, PM_NOREMOVE) &&
          !haveSentMessagesPending) {
        
        SwitchToThread();
      }
    }
  }

  
  UnhookWindowsHookEx(windowHook);

  
  
  UnhookNeuteredWindows();

  
  
  
  
  ScheduleDeferredMessageRun();

  if (timerId) {
    KillTimer(nullptr, timerId);
  }

  MessageChannel::SetIsPumpingMessages(false);

  return WaitResponse(timedout);
}

bool
MessageChannel::WaitForInterruptNotify()
{
  mMonitor->AssertCurrentThreadOwns();

  MOZ_ASSERT(gUIThreadId, "InitUIThread was not called!");

  
  
  if (!(mFlags & REQUIRE_DEFERRED_MESSAGE_PROTECTION)) {
    return WaitForSyncNotify();
  }

  if (!InterruptStackDepth() && !AwaitingIncomingMessage()) {
    
    NS_RUNTIMEABORT("StackDepth() is 0 in call to MessageChannel::WaitForNotify!");
  }

  NS_ASSERTION(mFlags & REQUIRE_DEFERRED_MESSAGE_PROTECTION,
               "Shouldn't be here for channels that don't use message deferral!");
  NS_ASSERTION(mTopFrame && mTopFrame->mInterrupt,
               "Top frame is not a sync frame!");

  MonitorAutoUnlock unlock(*mMonitor);

  bool timedout = false;

  UINT_PTR timerId = 0;
  TimeoutData timeoutData = { 0 };

  
  
  
  
  HHOOK windowHook = nullptr;

  while (1) {
    NS_ASSERTION((!!windowHook) == MessageChannel::IsPumpingMessages(),
                 "windowHook out of sync with reality");

    if (mTopFrame->mSpinNestedEvents) {
      if (windowHook) {
        UnhookWindowsHookEx(windowHook);
        windowHook = nullptr;

        if (timerId) {
          KillTimer(nullptr, timerId);
          timerId = 0;
        }

        
        MessageChannel::SetIsPumpingMessages(false);

        
        
        UnhookNeuteredWindows();

        
        
        
        ScheduleDeferredMessageRun();
      }
      SpinInternalEventLoop();
      ResetEvent(mEvent);
      return true;
    }

    if (!windowHook) {
      MessageChannel::SetIsPumpingMessages(true);
      windowHook = SetWindowsHookEx(WH_CALLWNDPROC, CallWindowProcedureHook,
                                    nullptr, gUIThreadId);
      NS_ASSERTION(windowHook, "Failed to set hook!");

      NS_ASSERTION(!timerId, "Timer already initialized?");

      if (mTimeoutMs != kNoTimeout) {
        InitTimeoutData(&timeoutData, mTimeoutMs);
        timerId = SetTimer(nullptr, 0, mTimeoutMs, nullptr);
        NS_ASSERTION(timerId, "SetTimer failed!");
      }
    }

    MSG msg = { 0 };

    
    {
      MonitorAutoLock lock(*mMonitor);
      if (!Connected()) {
        break;
      }
    }

    DWORD result = MsgWaitForMultipleObjects(1, &mEvent, FALSE, INFINITE,
                                             QS_ALLINPUT);
    if (result == WAIT_OBJECT_0) {
      
      ResetEvent(mEvent);
      break;
    } else
    if (result != (WAIT_OBJECT_0 + 1)) {
      NS_ERROR("Wait failed!");
      break;
    }

    if (TimeoutHasExpired(timeoutData)) {
      
      timedout = true;
      break;
    }

    
    bool haveSentMessagesPending =
      (HIWORD(GetQueueStatus(QS_SENDMESSAGE)) & QS_SENDMESSAGE) != 0;

    
    if (gCOMWindow) {
        if (PeekMessageW(&msg, gCOMWindow, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
    }

    
    
    if (!PeekMessageW(&msg, nullptr, 0, 0, PM_NOREMOVE) &&
        !haveSentMessagesPending) {
      
      SwitchToThread();
    }
  }

  if (windowHook) {
    
    UnhookWindowsHookEx(windowHook);

    
    
    UnhookNeuteredWindows();

    
    
    
    
    ScheduleDeferredMessageRun();

    if (timerId) {
      KillTimer(nullptr, timerId);
    }
  }

  MessageChannel::SetIsPumpingMessages(false);

  return WaitResponse(timedout);
}

void
MessageChannel::NotifyWorkerThread()
{
  mMonitor->AssertCurrentThreadOwns();

  if (mIsSyncWaitingOnNonMainThread) {
    mMonitor->Notify();
    return;
  }

  NS_ASSERTION(mEvent, "No signal event to set, this is really bad!");
  if (!SetEvent(mEvent)) {
    NS_WARNING("Failed to set NotifyWorkerThread event!");
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
  RedrawWindow(hWnd, nullptr, nullptr, flags);
  NS_ASSERTION(ret, "RedrawWindow failed!");
}

DeferredUpdateMessage::DeferredUpdateMessage(HWND aHWnd)
{
  mWnd = aHWnd;
  if (!GetUpdateRect(mWnd, &mUpdateRect, FALSE)) {
    memset(&mUpdateRect, 0, sizeof(RECT));
    return;
  }
  ValidateRect(mWnd, &mUpdateRect);
}

void
DeferredUpdateMessage::Run()
{
  AssertWindowIsNotNeutered(mWnd);
  if (!IsWindow(mWnd)) {
    NS_ERROR("Invalid window!");
    return;
  }

  InvalidateRect(mWnd, &mUpdateRect, FALSE);
#ifdef DEBUG
  BOOL ret =
#endif
  UpdateWindow(mWnd);
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
    lParamString = nullptr;
    lParam = 0;
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
      windowPos.hwndInsertAfter = nullptr;
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
    copyData.lpData = nullptr;
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
