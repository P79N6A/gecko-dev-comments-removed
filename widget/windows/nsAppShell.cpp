




#include "mozilla/ipc/MessageChannel.h"
#include "mozilla/ipc/WindowsMessageLoop.h"
#include "nsAppShell.h"
#include "nsToolkit.h"
#include "nsThreadUtils.h"
#include "WinUtils.h"
#include "WinTaskbar.h"
#include "WinMouseScrollHandler.h"
#include "nsWindowDefs.h"
#include "nsString.h"
#include "WinIMEHandler.h"
#include "mozilla/widget/AudioSession.h"
#include "mozilla/HangMonitor.h"
#include "nsIDOMWakeLockListener.h"
#include "nsIPowerManagerService.h"
#include "mozilla/StaticPtr.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "GeckoProfiler.h"

using namespace mozilla;
using namespace mozilla::widget;




class WinWakeLockListener : public nsIDOMMozWakeLockListener {
public:
  NS_DECL_ISUPPORTS;

private:
  ~WinWakeLockListener() {}

  NS_IMETHOD Callback(const nsAString& aTopic, const nsAString& aState) {
    if (!aTopic.EqualsASCII("screen")) {
      return NS_OK;
    }
    
    
    if (aState.EqualsASCII("locked-foreground")) {
      
      SetThreadExecutionState(ES_DISPLAY_REQUIRED|ES_CONTINUOUS);
    } else {
      
      SetThreadExecutionState(ES_CONTINUOUS);
    }
    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS(WinWakeLockListener, nsIDOMMozWakeLockListener)
StaticRefPtr<WinWakeLockListener> sWakeLockListener;

static void
AddScreenWakeLockListener()
{
  nsCOMPtr<nsIPowerManagerService> sPowerManagerService = do_GetService(POWERMANAGERSERVICE_CONTRACTID);
  if (sPowerManagerService) {
    sWakeLockListener = new WinWakeLockListener();
    sPowerManagerService->AddWakeLockListener(sWakeLockListener);
  } else {
    NS_WARNING("Failed to retrieve PowerManagerService, wakelocks will be broken!");
  }
}

static void
RemoveScreenWakeLockListener()
{
  nsCOMPtr<nsIPowerManagerService> sPowerManagerService = do_GetService(POWERMANAGERSERVICE_CONTRACTID);
  if (sPowerManagerService) {
    sPowerManagerService->RemoveWakeLockListener(sWakeLockListener);
    sPowerManagerService = nullptr;
    sWakeLockListener = nullptr;
  }
}

namespace mozilla {
namespace widget {

UINT sAppShellGeckoMsgId = RegisterWindowMessageW(L"nsAppShell:EventID");
} }

const wchar_t* kTaskbarButtonEventId = L"TaskbarButtonCreated";
UINT sTaskbarButtonCreatedMsg;


UINT nsAppShell::GetTaskbarButtonCreatedMessage() {
	return sTaskbarButtonCreatedMsg;
}

namespace mozilla {
namespace crashreporter {
void LSPAnnotate();
} 
} 

using mozilla::crashreporter::LSPAnnotate;



 LRESULT CALLBACK
nsAppShell::EventWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if (uMsg == sAppShellGeckoMsgId) {
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
#ifdef MOZ_CRASHREPORTER
  LSPAnnotate();
#endif

  mLastNativeEventScheduled = TimeStamp::NowLoRes();

  mozilla::ipc::windows::InitUIThread();

  sTaskbarButtonCreatedMsg = ::RegisterWindowMessageW(kTaskbarButtonEventId);
  NS_ASSERTION(sTaskbarButtonCreatedMsg, "Could not register taskbar button creation message");

  WNDCLASSW wc;
  HINSTANCE module = GetModuleHandle(nullptr);

  const wchar_t *const kWindowClass = L"nsAppShell:EventWindowClass";
  if (!GetClassInfoW(module, kWindowClass, &wc)) {
    wc.style         = 0;
    wc.lpfnWndProc   = EventWindowProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = module;
    wc.hIcon         = nullptr;
    wc.hCursor       = nullptr;
    wc.hbrBackground = (HBRUSH) nullptr;
    wc.lpszMenuName  = (LPCWSTR) nullptr;
    wc.lpszClassName = kWindowClass;
    RegisterClassW(&wc);
  }

  mEventWnd = CreateWindowW(kWindowClass, L"nsAppShell:EventWindow",
                           0, 0, 0, 10, 10, nullptr, nullptr, module, nullptr);
  NS_ENSURE_STATE(mEventWnd);

  return nsBaseAppShell::Init();
}

NS_IMETHODIMP
nsAppShell::Run(void)
{
  
  
  mozilla::widget::StartAudioSession();

  
  
  AddScreenWakeLockListener();

  nsresult rv = nsBaseAppShell::Run();

  RemoveScreenWakeLockListener();

  mozilla::widget::StopAudioSession();

  return rv;
}

NS_IMETHODIMP
nsAppShell::Exit(void)
{
  return nsBaseAppShell::Exit();
}

void
nsAppShell::DoProcessMoreGeckoEvents()
{
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  

  
  
  
  
  
  
  if (mEventloopNestingLevel < 2) {
    OnDispatchedEvent(nullptr);
    mNativeCallbackPending = false;
  } else {
    mNativeCallbackPending = true;
  }
}

void
nsAppShell::ScheduleNativeEventCallback()
{
  
  NS_ADDREF_THIS(); 
  {
    MutexAutoLock lock(mLastNativeEventScheduledMutex);
    
    
    mLastNativeEventScheduled = TimeStamp::NowLoRes();
  }
  ::PostMessage(mEventWnd, sAppShellGeckoMsgId, 0, reinterpret_cast<LPARAM>(this));
}

bool
nsAppShell::ProcessNextNativeEvent(bool mayWait)
{
  
  mozilla::ipc::MessageChannel::NotifyGeckoEventDispatch();

  bool gotMessage = false;

  do {
    MSG msg;
    bool uiMessage = false;

    
    
    
    
    
    
    
    if (mozilla::widget::MouseScrollHandler::IsWaitingInternalMessage()) {
      gotMessage = WinUtils::PeekMessage(&msg, nullptr, MOZ_WM_MOUSEWHEEL_FIRST,
                                         MOZ_WM_MOUSEWHEEL_LAST, PM_REMOVE);
      NS_ASSERTION(gotMessage,
                   "waiting internal wheel message, but it has not come");
      uiMessage = gotMessage;
    }

    if (!gotMessage) {
      gotMessage = WinUtils::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
      uiMessage =
        (msg.message >= WM_KEYFIRST && msg.message <= WM_IME_KEYLAST) ||
        (msg.message >= NS_WM_IMEFIRST && msg.message <= NS_WM_IMELAST) ||
        (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST);
    }

    if (gotMessage) {
      if (msg.message == WM_QUIT) {
        ::PostQuitMessage(msg.wParam);
        Exit();
      } else {
        
        
        mozilla::HangMonitor::NotifyActivity(
          uiMessage ? mozilla::HangMonitor::kUIActivity :
                      mozilla::HangMonitor::kActivityNoUIAVail);

        if (msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST &&
            IMEHandler::ProcessRawKeyMessage(msg)) {
          continue;  
        }

        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
      }
    } else if (mayWait) {
      
      mozilla::HangMonitor::Suspend();
      {
        GeckoProfilerSleepRAII profiler_sleep;
        WinUtils::WaitForMessage();
      }
    }
  } while (!gotMessage && mayWait);

  
  
  if (mNativeCallbackPending && mEventloopNestingLevel == 1)
    DoProcessMoreGeckoEvents();

  
  
  static const mozilla::TimeDuration nativeEventStarvationLimit =
    mozilla::TimeDuration::FromSeconds(NATIVE_EVENT_STARVATION_LIMIT);

  TimeDuration timeSinceLastNativeEventScheduled;
  {
    MutexAutoLock lock(mLastNativeEventScheduledMutex);
    timeSinceLastNativeEventScheduled =
        TimeStamp::NowLoRes() - mLastNativeEventScheduled;
  }
  if (timeSinceLastNativeEventScheduled > nativeEventStarvationLimit) {
    ScheduleNativeEventCallback();
  }

  return gotMessage;
}
