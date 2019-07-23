









































#include "nsIGenericFactory.h"
#include "nsIModule.h"
#include "nsCOMPtr.h"
#include "nsWidgetsCID.h"

#include "nsWindow.h"
#include "nsPopupWindow.h"
#include "nsChildView.h"
#include "nsSound.h"
#include "nsSystemSoundService.h"
#include "nsToolkit.h"
#include "nsAppShell.h"
#include "nsAppShellSingleton.h"
#include "nsLookAndFeel.h"
#include "nsFilePicker.h"
#include "nsBidiKeyboard.h"
#include "nsScreenManagerBeOS.h" 


#include "nsDeviceContextSpecB.h"
#include "nsPrintOptionsBeOS.h"
#include "nsPrintSession.h"


#include "nsTransferable.h"
#include "nsClipboard.h"
#include "nsClipboardHelper.h"
#include "nsHTMLFormatConverter.h"
#include "nsDragService.h"


NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsPopupWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsChildView)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsToolkit)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLookAndFeel)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTransferable)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboardHelper)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsHTMLFormatConverter)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDragService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSound)
NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsSystemSoundService,
                                         nsSystemSoundService::GetInstance)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFilePicker)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBidiKeyboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsScreenManagerBeOS)
 
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextSpecBeOS) 
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrintOptionsBeOS, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrintSession, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsPrinterEnumeratorBeOS)

static const nsModuleComponentInfo components[] =
{
  { "BeOS nsWindow",
    NS_WINDOW_CID,
    "@mozilla.org/widgets/window/beos;1",
    nsWindowConstructor },
  { "BeOS Popup nsWindow",
    NS_POPUP_CID,
    "@mozilla.org/widgets/popup/beos;1",
    nsPopupWindowConstructor },
  { "BeOS Child nsWindow",
    NS_CHILD_CID,
    "@mozilla.org/widgets/view/beos;1",
    nsChildViewConstructor },
  { "BeOS AppShell",
    NS_APPSHELL_CID,
    "@mozilla.org/widget/appshell/beos;1",
    nsAppShellConstructor },
  { "BeOS Toolkit",
    NS_TOOLKIT_CID,
    "@mozilla.org/widget/toolkit/beos;1",
    nsToolkitConstructor },
  { "BeOS Look And Feel",
    NS_LOOKANDFEEL_CID,
    "@mozilla.org/widget/lookandfeel;1",
    nsLookAndFeelConstructor },
  { "Transferrable",
    NS_TRANSFERABLE_CID,
    "@mozilla.org/widget/transferable;1",
    nsTransferableConstructor },
  { "BeOS Clipboard",
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
  { "BeOS Sound",
    NS_SOUND_CID,
    "@mozilla.org/sound;1",
    nsSoundConstructor },
  { "System Sound Service",
    NS_SYSTEM_SOUND_SERVICE_CID,
    "@mozilla.org/systemsoundservice;1",
    nsSystemSoundServiceConstructor },
  { "BeOS Drag Service",
    NS_DRAGSERVICE_CID,
    "@mozilla.org/widget/dragservice;1",
    nsDragServiceConstructor },
  { "BeOS Bidi Keyboard",
    NS_BIDIKEYBOARD_CID,
    "@mozilla.org/widget/bidikeyboard;1",
    nsBidiKeyboardConstructor },
  { "BeOS File Picker",
    NS_FILEPICKER_CID,
    "@mozilla.org/filepicker;1",
   nsFilePickerConstructor },
  { "BeOS Screen Manager", 
    NS_SCREENMANAGER_CID, 
    "@mozilla.org/gfx/screenmanager;1", 
    nsScreenManagerBeOSConstructor },
  { "BeOS Device Context Spec", 
    NS_DEVICE_CONTEXT_SPEC_CID, 
    
    "@mozilla.org/gfx/devicecontextspec;1", 
    nsDeviceContextSpecBeOSConstructor }, 
  { "BeOS Printer Enumerator",
    NS_PRINTER_ENUMERATOR_CID,
    
    "@mozilla.org/gfx/printerenumerator;1",
    nsPrinterEnumeratorBeOSConstructor },
  { "BeOS PrintSettings Service",
    NS_PRINTSETTINGSSERVICE_CID,
    "@mozilla.org/gfx/printsettings-service;1",
    nsPrintOptionsBeOSConstructor },
  { "Print Session",
    NS_PRINTSESSION_CID,
    "@mozilla.org/gfx/printsession;1",
    nsPrintSessionConstructor }
};

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(nsWidgetBeOSModule,
                              components,
                              nsAppShellInit, nsAppShellShutdown)
