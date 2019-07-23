







































#include "nsIGenericFactory.h"
#include "nsIModule.h"
#include "nsCOMPtr.h"
#include "nsWidgetsCID.h"

#include "nsWindow.h"
#include "nsAppShell.h"
#include "nsAppShellSingleton.h"
#include "nsToolkit.h"
#include "nsLookAndFeel.h"
#include "nsTransferable.h"
#include "nsClipboard.h"
#include "nsClipboardHelper.h"
#include "nsHTMLFormatConverter.h"
#include "nsDragService.h"
#include "nsSound.h"
#include "nsBidiKeyboard.h"
#include "nsScreenManagerGtk.h"
#ifdef NATIVE_THEME_SUPPORT
#include "nsNativeThemeGTK.h"
#endif

#include "nsGtkIMEHelper.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(ChildWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsToolkit)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLookAndFeel)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTransferable)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboardHelper)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsHTMLFormatConverter)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDragService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSound)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBidiKeyboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsScreenManagerGtk)
#ifdef NATIVE_THEME_SUPPORT
NS_GENERIC_FACTORY_CONSTRUCTOR(nsNativeThemeGTK)
#endif

static const nsModuleComponentInfo components[] =
{
  { "Gtk nsWindow",
    NS_WINDOW_CID,
    "@mozilla.org/widgets/window/gtk;1",
    nsWindowConstructor },
  { "Gtk Child nsWindow",
    NS_CHILD_CID,
    "@mozilla.org/widgets/child_window/gtk;1",
    ChildWindowConstructor },
  { "Gtk AppShell",
    NS_APPSHELL_CID,
    "@mozilla.org/widget/appshell/gtk;1",
    nsAppShellConstructor },
  { "Gtk Toolkit",
    NS_TOOLKIT_CID,
    "@mozilla.org/widget/toolkit/gtk;1",
    nsToolkitConstructor },
  { "Gtk Look And Feel",
    NS_LOOKANDFEEL_CID,
    "@mozilla.org/widget/lookandfeel;1",
    nsLookAndFeelConstructor },
  { "Gtk Sound",
    NS_SOUND_CID,
    
    "@mozilla.org/sound;1",
    nsSoundConstructor },
  { "Transferable",
    NS_TRANSFERABLE_CID,
    
    "@mozilla.org/widget/transferable;1",
    nsTransferableConstructor },
  { "Gtk Clipboard",
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
  { "Gtk Drag Service",
    NS_DRAGSERVICE_CID,
    
    "@mozilla.org/widget/dragservice;1",
    nsDragServiceConstructor },
  { "Gtk Bidi Keyboard",
    NS_BIDIKEYBOARD_CID,
    "@mozilla.org/widget/bidikeyboard;1",
    nsBidiKeyboardConstructor },
  { "Gtk Screen Manager",
    NS_SCREENMANAGER_CID,
    "@mozilla.org/gfx/screenmanager;1",
    nsScreenManagerGtkConstructor },
#ifdef NATIVE_THEME_SUPPORT
   { "Native Theme Renderer",
    NS_THEMERENDERER_CID,
    "@mozilla.org/chrome/chrome-native-theme;1",
    nsNativeThemeGTKConstructor }
#endif
};

PR_STATIC_CALLBACK(void)
nsWidgetGTKModuleDtor(nsIModule *self)
{
  nsWindow::ReleaseGlobals();
  nsGtkIMEHelper::Shutdown();
  nsAppShellShutdown(self);
}

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(nsWidgetGTKModule,
                                   components,
                                   nsAppShellInit,
                                   nsWidgetGTKModuleDtor)
