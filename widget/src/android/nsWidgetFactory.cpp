





































#include "mozilla/ModuleUtils.h"

#include "nsCOMPtr.h"
#include "nsWidgetsCID.h"
#include "nsAppShell.h"
#include "nsToolkit.h"

#include "nsWindow.h"
#include "nsLookAndFeel.h"
#include "nsAppShellSingleton.h"
#include "nsScreenManagerAndroid.h"

#include "nsIdleServiceAndroid.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsToolkit)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLookAndFeel)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsScreenManagerAndroid)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsIdleServiceAndroid)

NS_DEFINE_NAMED_CID(NS_TOOLKIT_CID);
NS_DEFINE_NAMED_CID(NS_APPSHELL_CID);
NS_DEFINE_NAMED_CID(NS_WINDOW_CID);
NS_DEFINE_NAMED_CID(NS_CHILD_CID);
NS_DEFINE_NAMED_CID(NS_LOOKANDFEEL_CID);
NS_DEFINE_NAMED_CID(NS_SCREENMANAGER_CID);
NS_DEFINE_NAMED_CID(NS_IDLE_SERVICE_CID);

static const mozilla::Module::CIDEntry kWidgetCIDs[] = {
  { &kNS_WINDOW_CID, false, NULL, nsWindowConstructor },
  { &kNS_CHILD_CID, false, NULL, nsWindowConstructor },
  { &kNS_APPSHELL_CID, false, NULL, nsAppShellConstructor },
  { &kNS_TOOLKIT_CID, false, NULL, nsToolkitConstructor },
  { &kNS_LOOKANDFEEL_CID, false, NULL, nsLookAndFeelConstructor },
  { &kNS_SCREENMANAGER_CID, false, NULL, nsScreenManagerAndroidConstructor },
  { &kNS_IDLE_SERVICE_CID, false, NULL, nsIdleServiceAndroidConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kWidgetContracts[] = {
  { "@mozilla.org/widgets/window/android;1", &kNS_WINDOW_CID },
  { "@mozilla.org/widgets/child_window/android;1", &kNS_CHILD_CID },
  { "@mozilla.org/widget/appshell/android;1", &kNS_APPSHELL_CID },
  { "@mozilla.org/widget/toolkit/android;1", &kNS_TOOLKIT_CID },
  { "@mozilla.org/widget/lookandfeel/android;1", &kNS_LOOKANDFEEL_CID },
  { "@mozilla.org/gfx/screenmanager;1", &kNS_SCREENMANAGER_CID },
  { "@mozilla.org/widget/idleservice;1", &kNS_IDLE_SERVICE_CID },
  { NULL }
};

static void
nsWidgetAndroidModuleDtor()
{
    nsAppShellShutdown();
}

static const mozilla::Module kWidgetModule = {
    mozilla::Module::kVersion,
    kWidgetCIDs,
    kWidgetContracts,
    NULL,
    NULL,
    nsAppShellInit,
    nsWidgetAndroidModuleDtor
};

NSMODULE_DEFN(nsWidgetAndroidModule) = &kWidgetModule;
