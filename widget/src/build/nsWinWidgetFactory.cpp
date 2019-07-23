






































#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsdefs.h"
#include "nsWidgetsCID.h"

#include "nsFilePicker.h"
#include "nsLookAndFeel.h"
#include "nsToolkit.h"
#include "nsWindow.h"
#include "nsAppShell.h"
#include "nsAppShellSingleton.h"
#include "nsIServiceManager.h"
#include "nsSound.h"
#include "nsIdleServiceWin.h"

#include "nsBidiKeyboard.h"


#include "nsClipboard.h"
#include "nsClipboardHelper.h"
#include "nsTransferable.h"
#include "nsHTMLFormatConverter.h"
#include "nsDragService.h"
#include "nsNativeThemeWin.h"
#include "nsScreenManagerWin.h"
#include "nsIGenericFactory.h"

#ifdef NS_PRINTING
#include "nsDeviceContextSpecWin.h"
#include "nsPrintOptionsWin.h"
#include "nsPrintSession.h"
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(ChildWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboardHelper)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsNativeThemeWin)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFilePicker)

#ifdef NS_PRINTING
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSound)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDragService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBidiKeyboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsIdleServiceWin)
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR(nsToolkit)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLookAndFeel)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTransferable)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsHTMLFormatConverter)

NS_GENERIC_FACTORY_CONSTRUCTOR(nsScreenManagerWin)

#ifdef NS_PRINTING
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrintOptionsWin, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsPrinterEnumeratorWin)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrintSession, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextSpecWin)
#endif

static const nsModuleComponentInfo components[] =
{
  { "nsWindow",
    NS_WINDOW_CID,
    "@mozilla.org/widgets/window/win;1",
    nsWindowConstructor },
  { "Child nsWindow",
    NS_CHILD_CID,
    "@mozilla.org/widgets/child_window/win;1",
    ChildWindowConstructor },
  { "Clipboard",
    NS_CLIPBOARD_CID,
    "@mozilla.org/widget/clipboard;1",
    nsClipboardConstructor },
  { "Clipboard Helper",
    NS_CLIPBOARDHELPER_CID,
    "@mozilla.org/widget/clipboardhelper;1",
    nsClipboardHelperConstructor },
  { "File Picker",
    NS_FILEPICKER_CID,
    "@mozilla.org/filepicker;1",
    nsFilePickerConstructor },
#ifndef WINCE
  { "Sound",
    NS_SOUND_CID,
    "@mozilla.org/sound;1",
    nsSoundConstructor },
  { "Drag Service",
    NS_DRAGSERVICE_CID,
    "@mozilla.org/widget/dragservice;1",
    nsDragServiceConstructor },
  { "Bidi Keyboard",
    NS_BIDIKEYBOARD_CID,
    "@mozilla.org/widget/bidikeyboard;1",
    nsBidiKeyboardConstructor },
  { "User Idle Service",
    NS_IDLE_SERVICE_CID,
    "@mozilla.org/widget/idleservice;1",
    nsIdleServiceWinConstructor },
#endif
  { "Native Theme Renderer", 
    NS_THEMERENDERER_CID,
    "@mozilla.org/chrome/chrome-native-theme;1", 
    NS_NewNativeTheme
  },
  { "AppShell",
    NS_APPSHELL_CID,
    "@mozilla.org/widget/appshell/win;1",
    nsAppShellConstructor },
  { "Toolkit",
    NS_TOOLKIT_CID,
    "@mozilla.org/widget/toolkit/win;1",
    nsToolkitConstructor },
  { "Look And Feel",
    NS_LOOKANDFEEL_CID,
    "@mozilla.org/widget/lookandfeel;1",
    nsLookAndFeelConstructor },
  { "Transferable",
    NS_TRANSFERABLE_CID,
    "@mozilla.org/widget/transferable;1",
    nsTransferableConstructor },
  { "HTML Format Converter",
    NS_HTMLFORMATCONVERTER_CID,
    "@mozilla.org/widget/htmlformatconverter;1",
    nsHTMLFormatConverterConstructor },
  { "nsScreenManagerWin",
    NS_SCREENMANAGER_CID,
    "@mozilla.org/gfx/screenmanager;1",
    nsScreenManagerWinConstructor },
#ifdef NS_PRINTING
  { "nsPrintOptionsWin",
    NS_PRINTSETTINGSSERVICE_CID,
    "@mozilla.org/gfx/printsettings-service;1",
    nsPrintOptionsWinConstructor },
  { "Win Printer Enumerator",
    NS_PRINTER_ENUMERATOR_CID,
    "@mozilla.org/gfx/printerenumerator;1",
    nsPrinterEnumeratorWinConstructor },
  { "Print Session",
    NS_PRINTSESSION_CID,
    "@mozilla.org/gfx/printsession;1",
    nsPrintSessionConstructor },
  { "nsDeviceContextSpecWin",
    NS_DEVICE_CONTEXT_SPEC_CID,
    "@mozilla.org/gfx/devicecontextspec;1",
    nsDeviceContextSpecWinConstructor },
#endif
};

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(nsWidgetModule, components,
                                   nsAppShellInit, nsAppShellShutdown)
