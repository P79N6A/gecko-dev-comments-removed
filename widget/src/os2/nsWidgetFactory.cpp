























































#include "nsIGenericFactory.h"
#include "nsIModule.h"
#include "nsCOMPtr.h"
#include "nsWidgetsCID.h"




#include "nsAppShell.h"
#include "nsAppShellSingleton.h"
#include "nsBidiKeyboard.h"
#include "nsWindow.h"
#include "nsDragService.h"
#include "nsILocalFile.h"
#include "nsFilePicker.h"
#include "nsLookAndFeel.h"
#include "nsSound.h"
#include "nsSystemSoundService.h"
#include "nsToolkit.h"


#include "nsClipboard.h"
#include "nsClipboardHelper.h"
#include "nsTransferable.h"
#include "nsHTMLFormatConverter.h"

#include "nsScreenManagerOS2.h"
#include "nsRwsService.h"


#include "nsDeviceContextSpecOS2.h"
#include "nsPrintOptionsOS2.h"
#include "nsPrintSession.h"

#include "nsFrameWindow.h" 

#include "nsIdleServiceOS2.h"


NS_GENERIC_FACTORY_CONSTRUCTOR(nsBidiKeyboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboardHelper)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFilePicker)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFrameWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLookAndFeel)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSound)
NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsSystemSoundService,
                                         nsSystemSoundService::GetInstance)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsToolkit)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTransferable)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsHTMLFormatConverter)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDragService)

NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextSpecOS2)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrintOptionsOS2, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsPrinterEnumeratorOS2)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrintSession, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsScreenManagerOS2)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsIdleServiceOS2)


static const nsModuleComponentInfo components[] =
{
  { "OS/2 AppShell",
    NS_APPSHELL_CID,
    "@mozilla.org/widget/appshell/os2;1",
    nsAppShellConstructor },
  { "OS/2 Bidi Keyboard",
    NS_BIDIKEYBOARD_CID,
    "@mozilla.org/widget/bidikeyboard;1",
    nsBidiKeyboardConstructor },
  { "OS/2 Child Window",
    NS_CHILD_CID,
    "@mozilla.org/widget/child_window/os2;1",
    nsWindowConstructor },
  { "OS/2 Clipboard",
    NS_CLIPBOARD_CID,
    "@mozilla.org/widget/clipboard;1",
    nsClipboardConstructor },
  { "Clipboard Helper",
    NS_CLIPBOARDHELPER_CID,
    "@mozilla.org/widget/clipboardhelper;1",
    nsClipboardHelperConstructor },
  { "OS/2 Drag Service",
    NS_DRAGSERVICE_CID,
    "@mozilla.org/widget/dragservice;1",
    nsDragServiceConstructor },
  { "OS/2 File Picker",
    NS_FILEPICKER_CID,
    "@mozilla.org/filepicker;1",
    nsFilePickerConstructor },
  { "OS/2 Look And Feel",
    NS_LOOKANDFEEL_CID,
    "@mozilla.org/widget/lookandfeel;1",
    nsLookAndFeelConstructor },
  { "OS/2 Sound",
    NS_SOUND_CID,
    "@mozilla.org/sound;1",
    nsSoundConstructor },
  { "System Sound Service",
    NS_SYSTEM_SOUND_SERVICE_CID,
    "@mozilla.org/systemsoundservice;1",
    nsSystemSoundServiceConstructor },
  { "OS/2 Toolkit",
    NS_TOOLKIT_CID,
    "@mozilla.org/widget/toolkit/os2;1",
    nsToolkitConstructor },
  { "OS/2 Frame Window",
    NS_WINDOW_CID,
    "@mozilla.org/widget/window/os2;1",
    nsFrameWindowConstructor },
  { "OS/2 Transferable",
    NS_TRANSFERABLE_CID,
    "@mozilla.org/widget/transferable;1",
    nsTransferableConstructor },
  { "OS/2 HTML Format Converter",
    NS_HTMLFORMATCONVERTER_CID,
    "@mozilla.org/widget/htmlformatconverter;1",
    nsHTMLFormatConverterConstructor },
  { "nsScreenManagerWin",
    NS_SCREENMANAGER_CID,
    "@mozilla.org/gfx/screenmanager;1",
    nsScreenManagerOS2Constructor },
  { "OS/2 Device Context Spec",
    NS_DEVICE_CONTEXT_SPEC_CID,
    
    "@mozilla.org/gfx/devicecontextspec;1",
    nsDeviceContextSpecOS2Constructor },
  { "PrintSettings Service",
    NS_PRINTSETTINGSSERVICE_CID,
    
    "@mozilla.org/gfx/printsettings-service;1",
    nsPrintOptionsOS2Constructor },
  { "Print Session",
    NS_PRINTSESSION_CID,
    "@mozilla.org/gfx/printsession;1",
    nsPrintSessionConstructor },
  { "OS/2 Printer Enumerator",
    NS_PRINTER_ENUMERATOR_CID,
    
    "@mozilla.org/gfx/printerenumerator;1",
    nsPrinterEnumeratorOS2Constructor },
  { "Rws Service Interface",
    NS_RWSSERVICE_CID,
    NS_RWSSERVICE_CONTRACTID,
    nsRwsServiceConstructor },
  { "User Idle Service",
    NS_IDLE_SERVICE_CID,
    "@mozilla.org/widget/idleservice;1",
    nsIdleServiceOS2Constructor },
};

static void
nsWidgetOS2ModuleDtor(nsIModule *self)
{
  nsWindow::ReleaseGlobals();
  nsFilePicker::ReleaseGlobals();
  nsAppShellShutdown(self);
}

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(nsWidgetOS2Module,
                                   components,
                                   nsAppShellInit,
                                   nsWidgetOS2ModuleDtor)
