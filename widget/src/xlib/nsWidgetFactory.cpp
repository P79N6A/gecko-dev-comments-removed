







































#include "nsIGenericFactory.h"
#include "nsIModule.h"
#include "nsCOMPtr.h"
#include "nsWidgetsCID.h"

#include "nsWindow.h"
#include "nsAppShell.h"
#include "nsToolkit.h"
#include "nsLookAndFeel.h"
#include "nsTransferable.h"
#include "nsClipboard.h"
#include "nsClipboardHelper.h"
#include "nsHTMLFormatConverter.h"
#include "nsDragService.h"
#include "nsSound.h"
#include "nsScreenManagerXlib.h"

#include "nsBidiKeyboard.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(ChildWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAppShell)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsToolkit)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLookAndFeel)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTransferable)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboardHelper)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsHTMLFormatConverter)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDragService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSound)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBidiKeyboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsScreenManagerXlib)

static const nsModuleComponentInfo components[] =
{
  { "Xlib nsWindow",
    NS_WINDOW_CID,
    "@mozilla.org/widgets/window/xlib;1",
    nsWindowConstructor },
  { "Xlib Child nsWindow",
    NS_CHILD_CID,
    "@mozilla.org/widgets/child_window/xlib;1",
    ChildWindowConstructor },
  { "Xlib AppShell",
    NS_APPSHELL_CID,
    "@mozilla.org/widget/appshell/xlib;1",
    nsAppShellConstructor },
  { "Xlib Toolkit",
    NS_TOOLKIT_CID,
    "@mozilla.org/widget/toolkit/xlib;1",
    nsToolkitConstructor },
  { "Xlib Look And Feel",
    NS_LOOKANDFEEL_CID,
    "@mozilla.org/widget/lookandfeel;1",
    nsLookAndFeelConstructor },
  { "Xlib Sound",
    NS_SOUND_CID,
      "@mozilla.org/sound;1",
    nsSoundConstructor },
  { "Transferrable",
    NS_TRANSFERABLE_CID,
    
    "@mozilla.org/widget/transferable;1",
    nsTransferableConstructor },
  { "Xlib Clipboard",
    NS_CLIPBOARD_CID,
    
    "@mozilla.org/widget/clipboard;1",
    nsClipboardConstructor },
  { "Clipboard Helper",
    NS_CLIPBOARDHELPER_CID,
    "@mozilla.org/widget/clipboardhelper;1",
    nsClipboardHelperConstructor },
  { "HTML Format Converter",
    NS_HTMLFORMATCONVERTER_CID,
    "@mozilla.org/widget/htmlformatconverter;1",
    nsHTMLFormatConverterConstructor },
  { "Xlib Drag Service",
    NS_DRAGSERVICE_CID,
    
    "@mozilla.org/widget/dragservice;1",
    nsDragServiceConstructor },
  { "Xlib Bidi Keyboard",
    NS_BIDIKEYBOARD_CID,
    "@mozilla.org/widget/bidikeyboard;1",
    nsBidiKeyboardConstructor },
  { "Xlib Screen Manager",
    NS_SCREENMANAGER_CID,
    "@mozilla.org/gfx/screenmanager;1",
    nsScreenManagerXlibConstructor }
};

PR_STATIC_CALLBACK(void)
nsWidgetXLIBModuleDtor(nsIModule *self)
{
  nsClipboard::Shutdown();
}

NS_IMPL_NSGETMODULE_WITH_DTOR(nsWidgetXLIBModule,
                              components,
                              nsWidgetXLIBModuleDtor)
