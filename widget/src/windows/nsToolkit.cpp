





































#include "nsToolkit.h"
#include "nsAppShell.h"
#include "nsWindow.h"
#include "nsWidgetsCID.h"
#include "prmon.h"
#include "prtime.h"
#include "nsGUIEvent.h"
#include "nsIServiceManager.h"
#include "nsComponentManagerUtils.h"
#include "nsWidgetAtoms.h"
#include "nsWindowAPI.h"
#include <objbase.h>
#include <initguid.h>


#include <unknwn.h>

NS_IMPL_ISUPPORTS1(nsToolkit, nsIToolkit)


static PRBool gIsMovingWindow = PR_FALSE;



static HHOOK   nsMsgFilterHook = NULL;





static PRUintn gToolkitTLSIndex = 0;


HINSTANCE nsToolkit::mDllInstance = 0;
PRBool    nsToolkit::mIsWinXP     = PR_FALSE;
static PRBool dummy = nsToolkit::InitVersionInfo();

#if !defined(MOZ_STATIC_COMPONENT_LIBS) && !defined(MOZ_ENABLE_LIBXUL)




#if defined(__GNUC__)

extern "C" {
#endif



#ifndef WINCE
BOOL APIENTRY DllMain(  HINSTANCE hModule, 
                        DWORD reason, 
                        LPVOID lpReserved )
{
    switch( reason ) {
        case DLL_PROCESS_ATTACH:
            nsToolkit::Startup(hModule);
            break;

        case DLL_THREAD_ATTACH:
            break;
    
        case DLL_THREAD_DETACH:
            break;
    
        case DLL_PROCESS_DETACH:
            nsToolkit::Shutdown();
            break;

    }

    return TRUE;
}
#endif 

#if defined(__GNUC__)
} 
#endif

#endif




PRBool gThreadState = PR_FALSE;

struct ThreadInitInfo {
    PRMonitor *monitor;
    nsToolkit *toolkit;
};



#ifndef WINCE
LRESULT CALLBACK DetectWindowMove(int code, WPARAM wParam, LPARAM lParam)
{
    static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

    



    CWPSTRUCT* sysMsg = (CWPSTRUCT*)lParam;
    if (sysMsg) {
      nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
      NS_ASSERTION(appShell, "no appshell");
      if (sysMsg->message == WM_ENTERSIZEMOVE) {
        gIsMovingWindow = PR_TRUE; 
        
        
        
        appShell->FavorPerformanceHint(PR_FALSE, 0);
      } else if (sysMsg->message == WM_EXITSIZEMOVE) {
        gIsMovingWindow = PR_FALSE;
        
        
        
        appShell->FavorPerformanceHint(PR_TRUE, 0);
      }
    }
    return CallNextHookEx(nsMsgFilterHook, code, wParam, lParam);
}
#endif 

MouseTrailer*       nsToolkit::gMouseTrailer;

void RunPump(void* arg)
{
    ThreadInitInfo *info = (ThreadInitInfo*)arg;
    ::PR_EnterMonitor(info->monitor);

    
    info->toolkit->CreateInternalWindow(PR_GetCurrentThread());

    gThreadState = PR_TRUE;

    ::PR_Notify(info->monitor);
    ::PR_ExitMonitor(info->monitor);

    delete info;

    
    MSG msg;
    while (::GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }
}






nsToolkit::nsToolkit()  
{
    mGuiThread  = NULL;
    mDispatchWnd = 0;

#if defined(MOZ_STATIC_COMPONENT_LIBS) || defined (WINCE)
    nsToolkit::Startup(GetModuleHandle(NULL));
#endif

    gMouseTrailer = new MouseTrailer();
}







nsToolkit::~nsToolkit()
{
    NS_PRECONDITION(::IsWindow(mDispatchWnd), "Invalid window handle");

    
    ::DestroyWindow(mDispatchWnd);
    mDispatchWnd = NULL;

    
    PR_SetThreadPrivate(gToolkitTLSIndex, nsnull);

    if (gMouseTrailer) {
      gMouseTrailer->DestroyTimer();
      delete gMouseTrailer;
      gMouseTrailer = nsnull;
    }

    
    
#ifndef WINCE
    if (nsMsgFilterHook != NULL) {
      UnhookWindowsHookEx(nsMsgFilterHook);
      nsMsgFilterHook = NULL;
    }
#endif

#if defined (MOZ_STATIC_COMPONENT_LIBS) || defined(WINCE)
    nsToolkit::Shutdown();
#endif
}

void
nsToolkit::Startup(HMODULE hModule)
{
    nsToolkit::mDllInstance = hModule;

    
    
    
    WNDCLASSW wc;
    wc.style            = CS_GLOBALCLASS;
    wc.lpfnWndProc      = nsToolkit::WindowProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = nsToolkit::mDllInstance;
    wc.hIcon            = NULL;
    wc.hCursor          = NULL;
    wc.hbrBackground    = NULL;
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = L"nsToolkitClass";
    VERIFY(::RegisterClassW(&wc));

    
    typedef BOOL (*SetProcessDPIAwareFunc)(VOID);

    SetProcessDPIAwareFunc setDPIAware = (SetProcessDPIAwareFunc)
      GetProcAddress(LoadLibrary("user32.dll"),
                     "SetProcessDPIAware");

    if (setDPIAware)
      setDPIAware();
}


void
nsToolkit::Shutdown()
{
    
    ::UnregisterClassW(L"nsToolkitClass", nsToolkit::mDllInstance);
}







void nsToolkit::CreateInternalWindow(PRThread *aThread)
{
    
    NS_PRECONDITION(aThread, "null thread");
    mGuiThread  = aThread;

    
    
    

    mDispatchWnd = ::CreateWindow("nsToolkitClass",
                                  "NetscapeDispatchWnd",
                                  WS_DISABLED,
                                  -50, -50,
                                  10, 10,
                                  NULL,
                                  NULL,
                                  nsToolkit::mDllInstance,
                                  NULL);

    VERIFY(mDispatchWnd);
}







void nsToolkit::CreateUIThread()
{
    PRMonitor *monitor = ::PR_NewMonitor();

    ::PR_EnterMonitor(monitor);

    ThreadInitInfo *ti = new ThreadInitInfo();
    ti->monitor = monitor;
    ti->toolkit = this;

    
    mGuiThread = ::PR_CreateThread(PR_SYSTEM_THREAD,
                                    RunPump,
                                    (void*)ti,
                                    PR_PRIORITY_NORMAL,
                                    PR_LOCAL_THREAD,
                                    PR_UNJOINABLE_THREAD,
                                    0);

    
    while(gThreadState == PR_FALSE) {
        ::PR_Wait(monitor, PR_INTERVAL_NO_TIMEOUT);
    }

    
    ::PR_ExitMonitor(monitor);
    ::PR_DestroyMonitor(monitor);
}






NS_METHOD nsToolkit::Init(PRThread *aThread)
{
    
    
    if (NULL != aThread) {
        CreateInternalWindow(aThread);
    } else {
        
        CreateUIThread();
    }

    nsWidgetAtoms::RegisterAtoms();

#ifndef WINCE
    
    
    if (nsMsgFilterHook == NULL) {
      nsMsgFilterHook = SetWindowsHookEx(WH_CALLWNDPROC, DetectWindowMove, 
                                         NULL, GetCurrentThreadId());
    }
#endif

    return NS_OK;
}

PRBool nsToolkit::UserIsMovingWindow(void)
{
    return gIsMovingWindow;
}






LRESULT CALLBACK nsToolkit::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, 
                                       LPARAM lParam)
{
    switch (msg) {
        case WM_CALLMETHOD:
        {
            MethodInfo *info = (MethodInfo *)lParam;
            return info->Invoke();
        }

        case WM_SYSCOLORCHANGE:
        {
          
          
          
          
          
          
          
          
          
          nsWindow::GlobalMsgWindowProc(hWnd, msg, wParam, lParam);
        }

    }

    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}









NS_METHOD NS_GetCurrentToolkit(nsIToolkit* *aResult)
{
  nsIToolkit* toolkit = nsnull;
  nsresult rv = NS_OK;
  PRStatus status;

  
  if (0 == gToolkitTLSIndex) {
    status = PR_NewThreadPrivateIndex(&gToolkitTLSIndex, NULL);
    if (PR_FAILURE == status) {
      rv = NS_ERROR_FAILURE;
    }
  }

  if (NS_SUCCEEDED(rv)) {
    toolkit = (nsIToolkit*)PR_GetThreadPrivate(gToolkitTLSIndex);

    
    
    
    if (!toolkit) {
      toolkit = new nsToolkit();

      if (!toolkit) {
        rv = NS_ERROR_OUT_OF_MEMORY;
      } else {
        NS_ADDREF(toolkit);
        toolkit->Init(PR_GetCurrentThread());
        
        
        
        
        PR_SetThreadPrivate(gToolkitTLSIndex, (void*)toolkit);
      }
    } else {
      NS_ADDREF(toolkit);
    }
    *aResult = toolkit;
  }

  return rv;
}


PRBool nsToolkit::InitVersionInfo()
{
  static PRBool isInitialized = PR_FALSE;

  if (!isInitialized)
  {
    isInitialized = PR_TRUE;

#ifndef WINCE
    OSVERSIONINFO osversion;
    osversion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    ::GetVersionEx(&osversion);

    if (osversion.dwMajorVersion == 5)  { 
      nsToolkit::mIsWinXP = (osversion.dwMinorVersion == 1);
    }
#endif
  }

  return PR_TRUE;
}





MouseTrailer::MouseTrailer() : mMouseTrailerWindow(nsnull), mCaptureWindow(nsnull),
  mIsInCaptureMode(PR_FALSE), mEnabled(PR_TRUE)
{
}




MouseTrailer::~MouseTrailer()
{
  DestroyTimer();
}




void MouseTrailer::SetMouseTrailerWindow(HWND aWnd) 
{
  if (mMouseTrailerWindow != aWnd && mTimer) {
    
    TimerProc(nsnull, nsnull);
  }
  mMouseTrailerWindow = aWnd;
  CreateTimer();
}





void MouseTrailer::SetCaptureWindow(HWND aWnd) 
{ 
  mCaptureWindow = aWnd;
  if (mCaptureWindow) {
    mIsInCaptureMode = PR_TRUE;
  }
}





nsresult MouseTrailer::CreateTimer()
{
  if (mTimer || !mEnabled) {
    return NS_OK;
  } 

  nsresult rv;
  mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return mTimer->InitWithFuncCallback(TimerProc, nsnull, 200,
                                      nsITimer::TYPE_REPEATING_SLACK);
}





void MouseTrailer::DestroyTimer()
{
  if (mTimer) {
    mTimer->Cancel();
    mTimer = nsnull;
  }
}





void MouseTrailer::TimerProc(nsITimer* aTimer, void* aClosure)
{
  MouseTrailer *mtrailer = nsToolkit::gMouseTrailer;
  NS_ASSERTION(mtrailer, "MouseTrailer still firing after deletion!");

  
  
  
  
  
  if (mtrailer->mCaptureWindow) {
    if (mtrailer->mCaptureWindow != mtrailer->mMouseTrailerWindow) {
      return;
    }
  } else {
    if (mtrailer->mIsInCaptureMode) {
      
      
      
      mtrailer->mMouseTrailerWindow = nsnull;
      mtrailer->mIsInCaptureMode = PR_FALSE;
      return;
    }
  }

  if (mtrailer->mMouseTrailerWindow && ::IsWindow(mtrailer->mMouseTrailerWindow)) {
    POINT mp;
    DWORD pos = ::GetMessagePos();
    mp.x = GET_X_LPARAM(pos);
    mp.y = GET_Y_LPARAM(pos);
    HWND mouseWnd = ::WindowFromPoint(mp);
    if (mtrailer->mMouseTrailerWindow != mouseWnd) {
#ifndef WINCE
      
      PostMessage(mtrailer->mMouseTrailerWindow, WM_MOUSELEAVE, NULL, NULL);
#endif

      
      mtrailer->DestroyTimer();
      mtrailer->mMouseTrailerWindow = nsnull;
    }
  } else {
    mtrailer->DestroyTimer();
    mtrailer->mMouseTrailerWindow = nsnull;
  }
}

