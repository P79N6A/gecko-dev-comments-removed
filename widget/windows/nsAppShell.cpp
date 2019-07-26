




#include "mozilla/ipc/RPCChannel.h"
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

using namespace mozilla::widget;

const PRUnichar* kAppShellEventId = L"nsAppShell:EventID";
const PRUnichar* kTaskbarButtonEventId = L"TaskbarButtonCreated";

static UINT sMsgId;

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
#ifdef MOZ_CRASHREPORTER
  LSPAnnotate();
#endif

  mLastNativeEventScheduled = TimeStamp::NowLoRes();

  if (!sMsgId)
    sMsgId = RegisterWindowMessageW(kAppShellEventId);

  sTaskbarButtonCreatedMsg = ::RegisterWindowMessageW(kTaskbarButtonEventId);
  NS_ASSERTION(sTaskbarButtonCreatedMsg, "Could not register taskbar button creation message");

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


NS_IMETHODIMP
nsAppShell::Run(void)
{
  
  
  mozilla::widget::StartAudioSession();

  nsresult rv = nsBaseAppShell::Run();

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
  
  
  mLastNativeEventScheduled = TimeStamp::NowLoRes();
  ::PostMessage(mEventWnd, sMsgId, 0, reinterpret_cast<LPARAM>(this));
}

bool
nsAppShell::ProcessNextNativeEvent(bool mayWait)
{
  
  mozilla::ipc::RPCChannel::NotifyGeckoEventDispatch();

  bool gotMessage = false;

  do {
    MSG msg;
    bool uiMessage = false;

    
    
    
    
    
    
    
    if (mozilla::widget::MouseScrollHandler::IsWaitingInternalMessage()) {
      gotMessage = WinUtils::PeekMessage(&msg, NULL, MOZ_WM_MOUSEWHEEL_FIRST,
                                         MOZ_WM_MOUSEWHEEL_LAST, PM_REMOVE);
      NS_ASSERTION(gotMessage,
                   "waiting internal wheel message, but it has not come");
      uiMessage = gotMessage;
    }

    if (!gotMessage) {
      gotMessage = WinUtils::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
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
      ::WaitMessage();
    }
  } while (!gotMessage && mayWait);

  
  
  if (mNativeCallbackPending && mEventloopNestingLevel == 1)
    DoProcessMoreGeckoEvents();

  
  
  static const mozilla::TimeDuration nativeEventStarvationLimit =
    mozilla::TimeDuration::FromSeconds(NATIVE_EVENT_STARVATION_LIMIT);

  if ((TimeStamp::NowLoRes() - mLastNativeEventScheduled) >
      nativeEventStarvationLimit) {
    ScheduleNativeEventCallback();
  }
  
  return gotMessage;
}
