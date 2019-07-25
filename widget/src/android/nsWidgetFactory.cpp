





































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
#include "nsClipboard.h"
#include "nsClipboardHelper.h"
#include "nsTransferable.h"
#include "nsPrintOptionsAndroid.h"
#include "nsPrintSession.h"
#include "nsDeviceContextAndroid.h"
#include "nsFilePicker.h"
#include "nsHTMLFormatConverter.h"
#include "nsIMEPicker.h"
#include "nsFilePickerProxy.h"
#include "nsXULAppAPI.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsToolkit)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLookAndFeel)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsScreenManagerAndroid)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsIdleServiceAndroid)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTransferable)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboardHelper)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrintOptionsAndroid, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrintSession, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextSpecAndroid)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsHTMLFormatConverter)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsIMEPicker)

#include "GfxInfo.h"
namespace mozilla {
namespace widget {

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(GfxInfo, Init)
}
}

static nsresult
nsFilePickerConstructor(nsISupports *aOuter, REFNSIID aIID,
                        void **aResult)
{
  *aResult = nsnull;
  if (aOuter != nsnull) {
    return NS_ERROR_NO_AGGREGATION;
  }
  nsCOMPtr<nsIFilePicker> picker;

  if (XRE_GetProcessType() == GeckoProcessType_Content)
    picker = new nsFilePickerProxy();
  else
    picker = new nsFilePicker;

  return picker->QueryInterface(aIID, aResult);
}

NS_DEFINE_NAMED_CID(NS_TOOLKIT_CID);
NS_DEFINE_NAMED_CID(NS_APPSHELL_CID);
NS_DEFINE_NAMED_CID(NS_WINDOW_CID);
NS_DEFINE_NAMED_CID(NS_CHILD_CID);
NS_DEFINE_NAMED_CID(NS_LOOKANDFEEL_CID);
NS_DEFINE_NAMED_CID(NS_SCREENMANAGER_CID);
NS_DEFINE_NAMED_CID(NS_IDLE_SERVICE_CID);
NS_DEFINE_NAMED_CID(NS_TRANSFERABLE_CID);
NS_DEFINE_NAMED_CID(NS_CLIPBOARD_CID);
NS_DEFINE_NAMED_CID(NS_CLIPBOARDHELPER_CID);
NS_DEFINE_NAMED_CID(NS_PRINTSETTINGSSERVICE_CID);
NS_DEFINE_NAMED_CID(NS_PRINTSESSION_CID);
NS_DEFINE_NAMED_CID(NS_DEVICE_CONTEXT_SPEC_CID);
NS_DEFINE_NAMED_CID(NS_FILEPICKER_CID);
NS_DEFINE_NAMED_CID(NS_HTMLFORMATCONVERTER_CID);
NS_DEFINE_NAMED_CID(NS_IMEPICKER_CID);
NS_DEFINE_NAMED_CID(NS_GFXINFO_CID);

static const mozilla::Module::CIDEntry kWidgetCIDs[] = {
  { &kNS_WINDOW_CID, false, NULL, nsWindowConstructor },
  { &kNS_CHILD_CID, false, NULL, nsWindowConstructor },
  { &kNS_APPSHELL_CID, false, NULL, nsAppShellConstructor },
  { &kNS_TOOLKIT_CID, false, NULL, nsToolkitConstructor },
  { &kNS_LOOKANDFEEL_CID, false, NULL, nsLookAndFeelConstructor },
  { &kNS_SCREENMANAGER_CID, false, NULL, nsScreenManagerAndroidConstructor },
  { &kNS_IDLE_SERVICE_CID, false, NULL, nsIdleServiceAndroidConstructor },
  { &kNS_TRANSFERABLE_CID, false, NULL, nsTransferableConstructor },
  { &kNS_CLIPBOARD_CID, false, NULL, nsClipboardConstructor },
  { &kNS_CLIPBOARDHELPER_CID, false, NULL, nsClipboardHelperConstructor },
  { &kNS_PRINTSETTINGSSERVICE_CID, false, NULL, nsPrintOptionsAndroidConstructor },
  { &kNS_PRINTSESSION_CID, false, NULL, nsPrintSessionConstructor },
  { &kNS_DEVICE_CONTEXT_SPEC_CID, false, NULL, nsDeviceContextSpecAndroidConstructor },
  { &kNS_FILEPICKER_CID, false, NULL, nsFilePickerConstructor },
  { &kNS_HTMLFORMATCONVERTER_CID, false, NULL, nsHTMLFormatConverterConstructor },
  { &kNS_IMEPICKER_CID, false, NULL, nsIMEPickerConstructor },
  { &kNS_GFXINFO_CID, false, NULL, mozilla::widget::GfxInfoConstructor },
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
  { "@mozilla.org/widget/transferable;1", &kNS_TRANSFERABLE_CID },
  { "@mozilla.org/widget/clipboard;1", &kNS_CLIPBOARD_CID },
  { "@mozilla.org/widget/clipboardhelper;1", &kNS_CLIPBOARDHELPER_CID },
  { "@mozilla.org/gfx/printsettings-service;1", &kNS_PRINTSETTINGSSERVICE_CID },
  { "@mozilla.org/gfx/printsession;1", &kNS_PRINTSESSION_CID },
  { "@mozilla.org/gfx/devicecontextspec;1", &kNS_DEVICE_CONTEXT_SPEC_CID },
  { "@mozilla.org/filepicker;1", &kNS_FILEPICKER_CID },
  { "@mozilla.org/widget/htmlformatconverter;1", &kNS_HTMLFORMATCONVERTER_CID },
  { "@mozilla.org/imepicker;1", &kNS_IMEPICKER_CID },
  { "@mozilla.org/gfx/info;1", &kNS_GFXINFO_CID },
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
