







































#include "mozilla/ipc/RPCChannel.h"
#include "nsAppShell.h"
#include "nsToolkit.h"
#include "nsThreadUtils.h"
#include "WinTaskbar.h"
#include "nsString.h"
#include "nsIMM32Handler.h"
#include "mozilla/widget/AudioSession.h"


#include <windows.h> 
#include <tlhelp32.h> 

const PRUnichar* kAppShellEventId = L"nsAppShell:EventID";
const PRUnichar* kTaskbarButtonEventId = L"TaskbarButtonCreated";

static UINT sMsgId;

#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_WIN7
static UINT sTaskbarButtonCreatedMsg;


UINT nsAppShell::GetTaskbarButtonCreatedMessage() {
	return sTaskbarButtonCreatedMsg;
}
#endif

namespace mozilla {
namespace crashreporter {
void LSPAnnotate();
} 
} 

using mozilla::crashreporter::LSPAnnotate;



static PRBool PeekUIMessage(MSG* aMsg)
{
  MSG keyMsg, imeMsg, mouseMsg, *pMsg = 0;
  PRBool haveKeyMsg, haveIMEMsg, haveMouseMsg;

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
    return PR_FALSE;
  }

  if (haveMouseMsg && (!pMsg || mouseMsg.time < pMsg->time)) {
    pMsg = &mouseMsg;
  }

  if (!pMsg) {
    return PR_FALSE;
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

  if (!sMsgId)
    sMsgId = RegisterWindowMessageW(kAppShellEventId);

#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_WIN7
  sTaskbarButtonCreatedMsg = ::RegisterWindowMessageW(kTaskbarButtonEventId);
  NS_ASSERTION(sTaskbarButtonCreatedMsg, "Could not register taskbar button creation message");

  
  
  mozilla::widget::WinTaskbar::RegisterAppUserModelID();
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

  
  
  PRBool done = !Module32FirstW(hModuleSnap, &module);
  while (!done) {
    NS_LossyConvertUTF16toASCII moduleName(module.szModule);
    PRBool found = PR_FALSE;
    PRUint32 i;
    for (i = 0; i < NUM_LOADEDMODULEINFO &&
                sLoadedModules[i].mStartAddr; ++i) {
      if (sLoadedModules[i].mStartAddr == module.modBaseAddr &&
          !strcmp(moduleName.get(),
                  sLoadedModules[i].mName)) {
        found = PR_TRUE;
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

  PRUint32 i;
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

#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
  
  
  mozilla::widget::StartAudioSession();
#endif

  nsresult rv = nsBaseAppShell::Run();

#ifdef MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
  mozilla::widget::StopAudioSession();
#endif

  
  sLoadedModules = nsnull;

  return rv;
}

#endif

void
nsAppShell::DoProcessMoreGeckoEvents()
{
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (mEventloopNestingLevel < 2) {
    OnDispatchedEvent(nsnull);
    mNativeCallbackPending = PR_FALSE;
  } else {
    mNativeCallbackPending = PR_TRUE;
  }
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
#if defined(_MSC_VER) && defined(_M_IX86)
  if (sXPCOMHasLoadedNewDLLs && sLoadedModules) {
    sXPCOMHasLoadedNewDLLs = PR_FALSE;
    CollectNewLoadedModules();
  }
#endif

  
  mozilla::ipc::RPCChannel::NotifyGeckoEventDispatch();

  PRBool gotMessage = PR_FALSE;

  do {
    MSG msg;
    
    if (PeekUIMessage(&msg) ||
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

  
  
  if (mNativeCallbackPending && mEventloopNestingLevel == 1)
    DoProcessMoreGeckoEvents();

  return gotMessage;
}
