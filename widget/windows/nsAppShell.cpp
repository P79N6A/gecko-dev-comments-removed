




#include "mozilla/ipc/RPCChannel.h"
#include "nsAppShell.h"
#include "nsToolkit.h"
#include "nsThreadUtils.h"
#include "WinTaskbar.h"
#include "WinMouseScrollHandler.h"
#include "nsWindowDefs.h"
#include "nsString.h"
#include "nsIMM32Handler.h"
#include "mozilla/widget/AudioSession.h"
#include "mozilla/HangMonitor.h"


#include <windows.h> 
#include <tlhelp32.h> 

const PRUnichar* kAppShellEventId = L"nsAppShell:EventID";
const PRUnichar* kTaskbarButtonEventId = L"TaskbarButtonCreated";


#define NATIVE_EVENT_STARVATION_LIMIT mozilla::TimeDuration::FromSeconds(1)

static UINT sMsgId;

static UINT sTaskbarButtonCreatedMsg;


UINT nsAppShell::GetTaskbarButtonCreatedMessage() {
	return sTaskbarButtonCreatedMsg;
}

namespace mozilla {
namespace crashreporter {
void LSPAnnotate();
} 
} 

using mozilla::crashreporter::LSPAnnotate;



static bool PeekUIMessage(MSG* aMsg)
{
  
  
  
  
  
  
  
  if (mozilla::widget::MouseScrollHandler::IsWaitingInternalMessage() &&
      ::PeekMessageW(aMsg, NULL, MOZ_WM_MOUSEWHEEL_FIRST,
                     MOZ_WM_MOUSEWHEEL_LAST, PM_REMOVE)) {
    return true;
  }

  MSG keyMsg, imeMsg, mouseMsg, *pMsg = 0;
  bool haveKeyMsg, haveIMEMsg, haveMouseMsg;

  haveKeyMsg = ::PeekMessageW(&keyMsg, NULL, WM_KEYFIRST, WM_IME_KEYLAST, PM_NOREMOVE);
  haveIMEMsg = ::PeekMessageW(&imeMsg, NULL, NS_WM_IMEFIRST, NS_WM_IMELAST, PM_NOREMOVE);
  haveMouseMsg = ::PeekMessageW(&mouseMsg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_NOREMOVE);

  if (haveKeyMsg) {
    pMsg = &keyMsg;
  }
  if (haveIMEMsg && (!pMsg || imeMsg.time < pMsg->time)) {
    pMsg = &imeMsg;
  }

  if (pMsg && !nsIMM32Handler::CanOptimizeKeyAndIMEMessages(pMsg)) {
    return false;
  }

  if (haveMouseMsg && (!pMsg || mouseMsg.time < pMsg->time)) {
    pMsg = &mouseMsg;
  }

  if (!pMsg) {
    return false;
  }

  return ::PeekMessageW(aMsg, NULL, pMsg->message, pMsg->message, PM_REMOVE);
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
#ifdef MOZ_CRASHREPORTER
  LSPAnnotate();
#endif

  mLastNativeEventScheduled = TimeStamp::Now();

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







#if defined(_MSC_VER) && defined(_M_IX86)

#define LOADEDMODULEINFO_STRSIZE 23
#define NUM_LOADEDMODULEINFO 250

struct LoadedModuleInfo {
  void* mStartAddr;
  void* mEndAddr;
  char mName[LOADEDMODULEINFO_STRSIZE + 1];
};

static LoadedModuleInfo* sLoadedModules = 0;

static void
CollectNewLoadedModules()
{
  HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
  MODULEENTRY32W module;

  
  hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
  if (hModuleSnap == INVALID_HANDLE_VALUE)
    return;

  
  module.dwSize = sizeof(MODULEENTRY32W);

  
  
  bool done = !Module32FirstW(hModuleSnap, &module);
  while (!done) {
    NS_LossyConvertUTF16toASCII moduleName(module.szModule);
    bool found = false;
    uint32_t i;
    for (i = 0; i < NUM_LOADEDMODULEINFO &&
                sLoadedModules[i].mStartAddr; ++i) {
      if (sLoadedModules[i].mStartAddr == module.modBaseAddr &&
          !strcmp(moduleName.get(),
                  sLoadedModules[i].mName)) {
        found = true;
        break;
      }
    }

    if (!found && i < NUM_LOADEDMODULEINFO) {
      sLoadedModules[i].mStartAddr = module.modBaseAddr;
      sLoadedModules[i].mEndAddr = module.modBaseAddr + module.modBaseSize;
      strncpy(sLoadedModules[i].mName, moduleName.get(),
              LOADEDMODULEINFO_STRSIZE);
      sLoadedModules[i].mName[LOADEDMODULEINFO_STRSIZE] = 0;
    }

    done = !Module32NextW(hModuleSnap, &module);
  }

  uint32_t i;
  for (i = 0; i < NUM_LOADEDMODULEINFO &&
              sLoadedModules[i].mStartAddr; ++i) {}

  CloseHandle(hModuleSnap);
}

NS_IMETHODIMP
nsAppShell::Run(void)
{
  LoadedModuleInfo modules[NUM_LOADEDMODULEINFO];
  memset(modules, 0, sizeof(modules));
  sLoadedModules = modules;	

  
  
  mozilla::widget::StartAudioSession();

  nsresult rv = nsBaseAppShell::Run();

  mozilla::widget::StopAudioSession();

  
  sLoadedModules = nullptr;

  return rv;
}

#endif

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
  
  
  mLastNativeEventScheduled = TimeStamp::Now();
  ::PostMessage(mEventWnd, sMsgId, 0, reinterpret_cast<LPARAM>(this));
}

bool
nsAppShell::ProcessNextNativeEvent(bool mayWait)
{
#if defined(_MSC_VER) && defined(_M_IX86)
  if (sXPCOMHasLoadedNewDLLs && sLoadedModules) {
    sXPCOMHasLoadedNewDLLs = false;
    CollectNewLoadedModules();
  }
#endif

  
  mozilla::ipc::RPCChannel::NotifyGeckoEventDispatch();

  bool gotMessage = false;

  do {
    MSG msg;
    bool uiMessage = PeekUIMessage(&msg);

    
    if (uiMessage ||
        PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
      gotMessage = true;
      if (msg.message == WM_QUIT) {
        ::PostQuitMessage(msg.wParam);
        Exit();
      } else {
        
        
        mozilla::HangMonitor::NotifyActivity(
          uiMessage ? mozilla::HangMonitor::kUIActivity :
                      mozilla::HangMonitor::kActivityNoUIAVail);
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

  
  
  if ((TimeStamp::Now() - mLastNativeEventScheduled) >
      NATIVE_EVENT_STARVATION_LIMIT) {
    ScheduleNativeEventCallback();
  }
  
  return gotMessage;
}
