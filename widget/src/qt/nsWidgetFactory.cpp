








































#include "nsIGenericFactory.h"
#include "nsIModule.h"

#include "nsCOMPtr.h"
#include "nsWidgetsCID.h"
#include "nsAppShell.h"
#include "nsWindow.h"
#include "nsToolkit.h"


























NS_GENERIC_FACTORY_CONSTRUCTOR(nsAppShell)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(ChildWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(PopupWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsToolkit)

















static const nsModuleComponentInfo components[] =
{
    { "Qt AppShell",
      NS_APPSHELL_CID,
      "@mozilla.org/widget/appshell/qt;1",
      nsAppShellConstructor },
    { "Qt Child nsWindow",
      NS_CHILD_CID,
      "@mozilla.org/widgets/child_window/qt;1",
      ChildWindowConstructor },
    { "Qt Popup nsWindow",
      NS_POPUP_CID,
      "@mozilla.org/widgets/popup_window/qt;1",
      PopupWindowConstructor },


















































    { "Qt nsWindow",
      NS_WINDOW_CID,
      "@mozilla.org/widgets/window/qt;1",
      nsWindowConstructor }
};

NS_IMPL_NSGETMODULE(nsWidgetQtModule,components)
