






































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
#ifdef PHOTON_DND
#include "nsDragService.h"
#endif
#include "nsSound.h"
#include "nsSystemSoundService.h"
#ifdef IBMBIDI
#include "nsBidiKeyboard.h"
#endif

#include "nsFilePicker.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(ChildWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAppShell)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsToolkit)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLookAndFeel)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTransferable)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboardHelper)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsHTMLFormatConverter)
#ifdef PHOTON_DND
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDragService)
#endif
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSound)
NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsSystemSoundService,
                                         nsSystemSoundService::GetInstance)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFilePicker)

#ifdef IBMBIDI
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBidiKeyboard)
#endif

static const nsModuleComponentInfo components[] =
{
  { "Ph nsWindow",
    NS_WINDOW_CID,
    "@mozilla.org/widgets/window/ph;1",
    nsWindowConstructor },
  { "Ph Child nsWindow",
    NS_CHILD_CID,
    "@mozilla.org/widgets/child_window/ph;1",
    ChildWindowConstructor },
  { "Ph AppShell",
    NS_APPSHELL_CID,
    "@mozilla.org/widget/appshell/ph;1",
    nsAppShellConstructor },
  { "Ph Toolkit",
    NS_TOOLKIT_CID,
    "@mozilla.org/widget/toolkit/ph;1",
    nsToolkitConstructor },
  { "Ph Look And Feel",
    NS_LOOKANDFEEL_CID,
    "@mozilla.org/widget/lookandfeel;1",
    nsLookAndFeelConstructor },
  { "Transferrable",
    NS_TRANSFERABLE_CID,
    "@mozilla.org/widget/transferable;1",
    nsTransferableConstructor },
  { "Ph Clipboard",
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
#ifdef PHOTON_DND
  { "Ph Drag Service",
    NS_DRAGSERVICE_CID,
    "@mozilla.org/widget/dragservice;1",
    nsDragServiceConstructor },
#endif
  { "Ph Sound",
    NS_SOUND_CID,
    "@mozilla.org/sound;1",
    nsSoundConstructor },
  { "System Sound Service",
    NS_SYSTEM_SOUND_SERVICE_CID,
    "@mozilla.org/systemsoundservice;1",
    nsSystemSoundServiceConstructor },
#ifdef IBMBIDI
    { "Gtk Bidi Keyboard",
    NS_BIDIKEYBOARD_CID,
    "@mozilla.org/widget/bidikeyboard;1",
    nsBidiKeyboardConstructor },
#endif 

  { "Photon File Picker",
    NS_FILEPICKER_CID,
    "@mozilla.org/filepicker;1",
    nsFilePickerConstructor },
};

static void
nsWidgetPhModuleDtor(nsIModule *self)
{

}

NS_IMPL_NSGETMODULE_WITH_DTOR(nsWidgetPhModule,
							components,
							nsWidgetPhModuleDtor)
