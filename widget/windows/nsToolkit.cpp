




#include "nsToolkit.h"
#include "nsAppShell.h"
#include "nsWindow.h"
#include "nsWidgetsCID.h"
#include "prmon.h"
#include "prtime.h"
#include "nsIServiceManager.h"
#include "nsComponentManagerUtils.h"
#include <objbase.h>
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






nsToolkit::nsToolkit()  
{
    MOZ_COUNT_CTOR(nsToolkit);

#if defined(MOZ_STATIC_COMPONENT_LIBS)
    nsToolkit::Startup(GetModuleHandle(nullptr));
#endif

    if (XRE_GetWindowsEnvironment() == WindowsEnvironmentType_Desktop) {
      mD3D9Timer = do_CreateInstance("@mozilla.org/timer;1");
      mD3D9Timer->InitWithFuncCallback(::StartAllowingD3D9,
                                       nullptr,
                                       kD3DUsageDelay,
                                       nsITimer::TYPE_ONE_SHOT);
    }
}







nsToolkit::~nsToolkit()
{
    MOZ_COUNT_DTOR(nsToolkit);
}

void
nsToolkit::Startup(HMODULE hModule)
{
    nsToolkit::mDllInstance = hModule;
    WinUtils::Initialize();
    nsUXThemeData::Initialize();
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
