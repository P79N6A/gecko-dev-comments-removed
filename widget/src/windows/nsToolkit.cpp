





































#include "nsToolkit.h"
#include "nsAppShell.h"
#include "nsWindow.h"
#include "nsWidgetsCID.h"
#include "prmon.h"
#include "prtime.h"
#include "nsGUIEvent.h"
#include "nsIServiceManager.h"
#include "nsComponentManagerUtils.h"
#include <objbase.h>
#include <initguid.h>

#include "nsUXThemeData.h"


#include <unknwn.h>

nsToolkit* nsToolkit::gToolkit = nsnull;

HINSTANCE nsToolkit::mDllInstance = 0;
bool      nsToolkit::mIsWinXP     = false;
static bool dummy = nsToolkit::InitVersionInfo();

static const unsigned long kD3DUsageDelay = 5000;

static void
StartAllowingD3D9(nsITimer *aTimer, void *aClosure)
{
  nsWindow::StartAllowingD3D9(true);
}




bool gThreadState = false;

struct ThreadInitInfo {
    PRMonitor *monitor;
    nsToolkit *toolkit;
};

MouseTrailer*       nsToolkit::gMouseTrailer;

void RunPump(void* arg)
{
    ThreadInitInfo *info = (ThreadInitInfo*)arg;
    ::PR_EnterMonitor(info->monitor);

    
    info->toolkit->CreateInternalWindow(PR_GetCurrentThread());

    gThreadState = true;

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
    MOZ_COUNT_CTOR(nsToolkit);
    mGuiThread  = NULL;
    mDispatchWnd = 0;

#if defined(MOZ_STATIC_COMPONENT_LIBS)
    nsToolkit::Startup(GetModuleHandle(NULL));
#endif

    gMouseTrailer = new MouseTrailer();

    
    
    PRThread* thread = PR_GetCurrentThread();
    if (NULL != thread) {
      CreateInternalWindow(thread);
    } else {
      
      CreateUIThread();
    }

    mD3D9Timer = do_CreateInstance("@mozilla.org/timer;1");
    mD3D9Timer->InitWithFuncCallback(::StartAllowingD3D9,
                                     NULL,
                                     kD3DUsageDelay,
                                     nsITimer::TYPE_ONE_SHOT);
}







nsToolkit::~nsToolkit()
{
    MOZ_COUNT_DTOR(nsToolkit);
    NS_PRECONDITION(::IsWindow(mDispatchWnd), "Invalid window handle");

    
    ::DestroyWindow(mDispatchWnd);
    mDispatchWnd = NULL;

    if (gMouseTrailer) {
      gMouseTrailer->DestroyTimer();
      delete gMouseTrailer;
      gMouseTrailer = nsnull;
    }
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
    VERIFY(::RegisterClassW(&wc) || 
           GetLastError() == ERROR_CLASS_ALREADY_EXISTS);

    nsUXThemeData::Initialize();
}


void
nsToolkit::Shutdown()
{
#if defined (MOZ_STATIC_COMPONENT_LIBS)
    
    
    
    ::UnregisterClassW(L"nsToolkitClass", nsToolkit::mDllInstance);
#endif

    delete gToolkit;
    gToolkit = nsnull;
}

void
nsToolkit::StartAllowingD3D9()
{
  nsToolkit::GetToolkit()->mD3D9Timer->Cancel();
  nsWindow::StartAllowingD3D9(false);
}






void nsToolkit::CreateInternalWindow(PRThread *aThread)
{
    
    NS_PRECONDITION(aThread, "null thread");
    mGuiThread  = aThread;

    
    
    

    mDispatchWnd = ::CreateWindowW(L"nsToolkitClass",
                                   L"NetscapeDispatchWnd",
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

    
    while(!gThreadState) {
        ::PR_Wait(monitor, PR_INTERVAL_NO_TIMEOUT);
    }

    
    ::PR_ExitMonitor(monitor);
    ::PR_DestroyMonitor(monitor);
}







LRESULT CALLBACK nsToolkit::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, 
                                       LPARAM lParam)
{
    switch (msg) {
        case WM_SYSCOLORCHANGE:
        {
          
          
          
          
          
          
          
          
          
          nsWindow::GlobalMsgWindowProc(hWnd, msg, wParam, lParam);
        }
    }

    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}










nsToolkit* nsToolkit::GetToolkit()
{
  if (!gToolkit) {
    gToolkit = new nsToolkit();
  }

  return gToolkit;
}


bool nsToolkit::InitVersionInfo()
{
  static bool isInitialized = false;

  if (!isInitialized)
  {
    isInitialized = true;

    OSVERSIONINFO osversion;
    osversion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    ::GetVersionEx(&osversion);

    if (osversion.dwMajorVersion == 5)  { 
      nsToolkit::mIsWinXP = (osversion.dwMinorVersion == 1);
    }
  }

  return true;
}





MouseTrailer::MouseTrailer() : mMouseTrailerWindow(nsnull), mCaptureWindow(nsnull),
  mIsInCaptureMode(false), mEnabled(true)
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
    mIsInCaptureMode = true;
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
      mtrailer->mIsInCaptureMode = false;
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
      
      PostMessage(mtrailer->mMouseTrailerWindow, WM_MOUSELEAVE, 0, 0);

      
      mtrailer->DestroyTimer();
      mtrailer->mMouseTrailerWindow = nsnull;
    }
  } else {
    mtrailer->DestroyTimer();
    mtrailer->mMouseTrailerWindow = nsnull;
  }
}

