





































#include "nsIGenericFactory.h"
#include "nsIModule.h"

#include "nsCOMPtr.h"
#include "nsWidgetsCID.h"
#include "nsAppShell.h"
#include "nsToolkit.h"

#include "nsWindow.h"
#include "nsLookAndFeel.h"
#include "nsAppShellSingleton.h"
#include "nsScreenManagerAndroid.h"

#include "nsAccelerometerAndroid.h"
#include "nsIdleServiceAndroid.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsToolkit)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLookAndFeel)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsScreenManagerAndroid)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAccelerometerAndroid)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsIdleServiceAndroid)


static const nsModuleComponentInfo components[] =
{
    { "Android Toolkit",
      NS_TOOLKIT_CID,
      "@mozilla.org/widget/toolkit/android;1",
      nsToolkitConstructor },
    { "Android AppShell",
      NS_APPSHELL_CID,
      "@mozilla.org/widget/appshell/android;1",
      nsAppShellConstructor },
    { "Android nsWindow",
      NS_WINDOW_CID,
      "@mozilla.org/widgets/window/android;1",
      nsWindowConstructor },
    { "Android Child nsWindow",
      NS_CHILD_CID,
      "@mozilla.org/widgets/child_window/android;1",
      nsWindowConstructor }, 
    { "Android Look And Feel",
      NS_LOOKANDFEEL_CID,
      "@mozilla.org/widget/lookandfeel/android;1",
      nsLookAndFeelConstructor },
    { "Android Screen Manager",
      NS_SCREENMANAGER_CID,
      "@mozilla.org/gfx/screenmanager;1",
      nsScreenManagerAndroidConstructor },
    { "Android Idle Service",
      NS_IDLE_SERVICE_CID,
      "@mozilla.org/widget/idleservice;1",
      nsIdleServiceAndroidConstructor },
    { "Accelerometer",
      NS_ACCELEROMETER_CID,
      NS_ACCELEROMETER_CONTRACTID,
      nsAccelerometerAndroidConstructor },

};

static void
nsWidgetAndroidModuleDtor(nsIModule *aSelf)
{
    nsAppShellShutdown(aSelf);
}

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(nsWidgetAndroidModule,
                                   components,
                                   nsAppShellInit,
                                   nsWidgetAndroidModuleDtor)

