





































#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsIComponentManager.h"
#include "nsIGenericFactory.h"

#include "nsWidgetsCID.h"

#include "nsToolkit.h"
#include "nsWindow.h"
#include "nsMacWindow.h"
#include "nsAppShell.h"
#include "nsAppShellSingleton.h"
#include "nsFilePicker.h"
#include "nsNativeScrollbar.h"

#include "nsMenuBarX.h"
#include "nsMenuX.h"
#include "nsMenuItemX.h"

#include "nsClipboard.h"
#include "nsClipboardHelper.h"
#include "nsTransferable.h"
#include "nsHTMLFormatConverter.h"
#include "nsDragService.h"
#include "nsDragHelperService.h"
#include "nsLookAndFeel.h"
#include "nsSound.h"
#include "nsBidiKeyboard.h"
#include "nsNativeThemeMac.h"
#include "nsScreenManagerMac.h"

#include "nsPrintOptionsX.h"
#include "nsPrintSessionX.h"
#include "nsDeviceContextSpecX.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsMacWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(ChildWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFilePicker)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsToolkit)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLookAndFeel)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMenuBarX)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMenuX)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMenuItemX)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSound)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTransferable)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboardHelper)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsHTMLFormatConverter)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDragService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDragHelperService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsNativeScrollbar)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBidiKeyboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsNativeThemeMac)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsScreenManagerMac)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrintOptionsX, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrintSessionX, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextSpecX)

static const nsModuleComponentInfo gComponents[] =
{
  { "nsWindow",
    NS_WINDOW_CID,
    "@mozilla.org/widgets/window/mac;1",
    nsMacWindowConstructor },
  { "Popup nsWindow",
    NS_POPUP_CID,
    "@mozilla.org/widgets/popup/mac;1",
    nsMacWindowConstructor },
  { "Child nsWindow",
    NS_CHILD_CID,
    "@mozilla.org/widgets/child_window/mac;1",
    ChildWindowConstructor },
  { "File Picker",
    NS_FILEPICKER_CID,
    "@mozilla.org/filepicker;1",
    nsFilePickerConstructor },
  { "AppShell",
    NS_APPSHELL_CID,
    "@mozilla.org/widget/appshell/mac;1",
    nsAppShellConstructor },
  { "Toolkit",
    NS_TOOLKIT_CID,
    "@mozilla.org/widget/toolkit/mac;1",
    nsToolkitConstructor },
  { "Look And Feel",
    NS_LOOKANDFEEL_CID,
    "@mozilla.org/widget/lookandfeel;1",
    nsLookAndFeelConstructor },
  { "Menubar",
    NS_MENUBAR_CID,
    "@mozilla.org/widget/menubar/mac;1",
    nsMenuBarXConstructor },
  { "Menu",
    NS_MENU_CID,
    "@mozilla.org/widget/menu/mac;1",
    nsMenuXConstructor },
  { "MenuItem",
    NS_MENUITEM_CID,
    "@mozilla.org/widget/menuitem/mac;1",
    nsMenuItemXConstructor },
  { "Sound",
    NS_SOUND_CID,
    "@mozilla.org/sound;1",
    nsSoundConstructor },
  {  "Transferable",
    NS_TRANSFERABLE_CID,
    "@mozilla.org/widget/transferable;1",
    nsTransferableConstructor },
  {  "HTML Format Converter",
    NS_HTMLFORMATCONVERTER_CID,
    "@mozilla.org/widget/htmlformatconverter;1",
    nsHTMLFormatConverterConstructor },
  {  "Clipboard",
    NS_CLIPBOARD_CID,
    "@mozilla.org/widget/clipboard;1",
    nsClipboardConstructor },
  {  "Clipboard Helper",
    NS_CLIPBOARDHELPER_CID,
    "@mozilla.org/widget/clipboardhelper;1",
    nsClipboardHelperConstructor },
  {  "Drag Service",
    NS_DRAGSERVICE_CID,
    "@mozilla.org/widget/dragservice;1",
    nsDragServiceConstructor },
  {  "Drag Helper Service",
    NS_DRAGHELPERSERVICE_CID,
    "@mozilla.org/widget/draghelperservice;1",
    nsDragHelperServiceConstructor },
  { "Native Scrollbar",
    NS_NATIVESCROLLBAR_CID,
    "@mozilla.org/widget/nativescrollbar;1",
    nsNativeScrollbarConstructor },
  { "Mac Bidi Keyboard",
    NS_BIDIKEYBOARD_CID,
    "@mozilla.org/widget/bidikeyboard;1",
    nsBidiKeyboardConstructor },
  {  "Native Theme Renderer", 
    NS_THEMERENDERER_CID,
    "@mozilla.org/chrome/chrome-native-theme;1",
    nsNativeThemeMacConstructor },
  { "nsScreenManager",
    NS_SCREENMANAGER_CID,
    "@mozilla.org/gfx/screenmanager;1",
    nsScreenManagerMacConstructor },
  { "nsDeviceContextSpec",
    NS_DEVICE_CONTEXT_SPEC_CID,
    "@mozilla.org/gfx/devicecontextspec;1",
    nsDeviceContextSpecXConstructor },
  { "PrintSettings Service",
    NS_PRINTSETTINGSSERVICE_CID,
    "@mozilla.org/gfx/printsettings-service;1",
    nsPrintOptionsXConstructor },
  { "Print Session",
    NS_PRINTSESSION_CID,
    "@mozilla.org/gfx/printsession;1",
    nsPrintSessionXConstructor },
};

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(nsWidgetMacModule, gComponents,
                                   nsAppShellInit, nsAppShellShutdown)
