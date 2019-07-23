








































#include "nsIGenericFactory.h"
#include "nsIModule.h"

#include "nsCOMPtr.h"
#include "nsWidgetsCID.h"
#include "nsAppShell.h"
#include "nsWindow.h"
#include "nsToolkit.h"
#include "nsHTMLFormatConverter.h"
#include "nsTransferable.h"

























NS_GENERIC_FACTORY_CONSTRUCTOR(nsAppShell)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(ChildWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(PopupWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsToolkit)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsHTMLFormatConverter)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTransferable)

















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
    { "HTML Format Converter",
      NS_HTMLFORMATCONVERTER_CID,
      "@mozilla.org/widget/htmlformatconverter/qt;1",
      nsHTMLFormatConverterConstructor },
    { "Qt Toolkit",
      NS_TOOLKIT_CID,
      "@mozilla.org/widget/toolkit/qt;1",
      nsToolkitConstructor },
    { "Transferrable",
      NS_TRANSFERABLE_CID,
      "@mozilla.org/widget/transferable;1",
      nsTransferableConstructor },






































    { "Qt nsWindow",
      NS_WINDOW_CID,
      "@mozilla.org/widgets/window/qt;1",
      nsWindowConstructor }
};

NS_IMPL_NSGETMODULE(nsWidgetQtModule,components)
