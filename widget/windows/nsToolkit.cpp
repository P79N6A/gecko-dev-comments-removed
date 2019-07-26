




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
#include "WinUtils.h"

#include "nsUXThemeData.h"


#include <unknwn.h>

using namespace mozilla::widget;

nsToolkit* nsToolkit::gToolkit = nullptr;
HINSTANCE nsToolkit::mDllInstance = 0;
static const unsigned long kD3DUsageDelay = 5000;

static void
StartAllowingD3D9(nsITimer *aTimer, void *aClosure)
{
  if (XRE_GetWindowsEnvironment() == WindowsEnvironmentType_Desktop) {
    nsWindow::StartAllowingD3D9(true);
  }
}

MouseTrailer*       nsToolkit::gMouseTrailer;






nsToolkit::nsToolkit()  
{
    MOZ_COUNT_CTOR(nsToolkit);

#if defined(MOZ_STATIC_COMPONENT_LIBS)
    nsToolkit::Startup(GetModuleHandle(NULL));
#endif

    gMouseTrailer = &mMouseTrailer;

    if (XRE_GetWindowsEnvironment() == WindowsEnvironmentType_Desktop) {
      mD3D9Timer = do_CreateInstance("@mozilla.org/timer;1");
      mD3D9Timer->InitWithFuncCallback(::StartAllowingD3D9,
                                       NULL,
                                       kD3DUsageDelay,
                                       nsITimer::TYPE_ONE_SHOT);
    }
}







nsToolkit::~nsToolkit()
{
    MOZ_COUNT_DTOR(nsToolkit);
    gMouseTrailer = nullptr;
}

void
nsToolkit::Startup(HMODULE hModule)
{
    nsToolkit::mDllInstance = hModule;
    nsUXThemeData::Initialize();
    WinUtils::Initialize();
}

void
nsToolkit::Shutdown()
{
    delete gToolkit;
    gToolkit = nullptr;
}

void
nsToolkit::StartAllowingD3D9()
{
  if (XRE_GetWindowsEnvironment() == WindowsEnvironmentType_Desktop) {
    nsToolkit::GetToolkit()->mD3D9Timer->Cancel();
    nsWindow::StartAllowingD3D9(false);
  }
}








nsToolkit* nsToolkit::GetToolkit()
{
  if (!gToolkit) {
    gToolkit = new nsToolkit();
  }

  return gToolkit;
}






MouseTrailer::MouseTrailer() : mMouseTrailerWindow(nullptr), mCaptureWindow(nullptr),
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
    
    TimerProc(nullptr, nullptr);
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

  return mTimer->InitWithFuncCallback(TimerProc, nullptr, 200,
                                      nsITimer::TYPE_REPEATING_SLACK);
}





void MouseTrailer::DestroyTimer()
{
  if (mTimer) {
    mTimer->Cancel();
    mTimer = nullptr;
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
      
      
      
      mtrailer->mMouseTrailerWindow = nullptr;
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
      mtrailer->mMouseTrailerWindow = nullptr;
    }
  } else {
    mtrailer->DestroyTimer();
    mtrailer->mMouseTrailerWindow = nullptr;
  }
}

