








































#include "nsWindow.h"
#include "nsAppShell.h"

#include "mozilla/ModuleUtils.h"
#include "nsIModule.h"

#include "nsCOMPtr.h"
#include "nsWidgetsCID.h"
#include "nsToolkit.h"
#include "nsHTMLFormatConverter.h"
#include "nsTransferable.h"
#include "nsLookAndFeel.h"
#include "nsAppShellSingleton.h"
#include "nsScreenManagerQt.h"
#include "nsFilePicker.h"
#include "nsClipboard.h"
#include "nsClipboardHelper.h"
#include "nsIdleServiceQt.h"
#include "nsDragService.h"
#include "nsSound.h"
#include "nsBidiKeyboard.h"
#include "nsNativeThemeQt.h"


extern PRBool gDisableNativeTheme;

NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsChildWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsPopupWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsToolkit)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsHTMLFormatConverter)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTransferable)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLookAndFeel)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsScreenManagerQt)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboardHelper)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDragService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBidiKeyboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsIdleServiceQt)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSound)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFilePicker)

static nsresult
nsNativeThemeQtConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult)
{
    nsresult rv;
    nsNativeThemeQt *inst;

    if (gDisableNativeTheme)
        return NS_ERROR_NO_INTERFACE;

    *aResult = NULL;
    if (NULL != aOuter)
        return NS_ERROR_NO_AGGREGATION;

    inst = new nsNativeThemeQt();
    if (NULL == inst)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(inst);
    rv = inst->QueryInterface(aIID, aResult);
    NS_RELEASE(inst);

    return rv;
}

NS_DEFINE_NAMED_CID(NS_WINDOW_CID);
NS_DEFINE_NAMED_CID(NS_CHILD_CID);
NS_DEFINE_NAMED_CID(NS_APPSHELL_CID);
NS_DEFINE_NAMED_CID(NS_LOOKANDFEEL_CID);
NS_DEFINE_NAMED_CID(NS_FILEPICKER_CID);
NS_DEFINE_NAMED_CID(NS_SOUND_CID);
NS_DEFINE_NAMED_CID(NS_TRANSFERABLE_CID);
NS_DEFINE_NAMED_CID(NS_CLIPBOARD_CID);
NS_DEFINE_NAMED_CID(NS_CLIPBOARDHELPER_CID);
NS_DEFINE_NAMED_CID(NS_DRAGSERVICE_CID);
NS_DEFINE_NAMED_CID(NS_HTMLFORMATCONVERTER_CID);
NS_DEFINE_NAMED_CID(NS_BIDIKEYBOARD_CID);
NS_DEFINE_NAMED_CID(NS_SCREENMANAGER_CID);
NS_DEFINE_NAMED_CID(NS_THEMERENDERER_CID);
NS_DEFINE_NAMED_CID(NS_IDLE_SERVICE_CID);
NS_DEFINE_NAMED_CID(NS_POPUP_CID);
NS_DEFINE_NAMED_CID(NS_TOOLKIT_CID);

static const mozilla::Module::CIDEntry kWidgetCIDs[] = {
    { &kNS_WINDOW_CID, false, NULL, nsWindowConstructor },
    { &kNS_CHILD_CID, false, NULL, nsChildWindowConstructor },
    { &kNS_APPSHELL_CID, false, NULL, nsAppShellConstructor },
    { &kNS_LOOKANDFEEL_CID, false, NULL, nsLookAndFeelConstructor },
    { &kNS_FILEPICKER_CID, false, NULL, nsFilePickerConstructor },
    { &kNS_SOUND_CID, false, NULL, nsSoundConstructor },
    { &kNS_TRANSFERABLE_CID, false, NULL, nsTransferableConstructor },
    { &kNS_CLIPBOARD_CID, false, NULL, nsClipboardConstructor },
    { &kNS_CLIPBOARDHELPER_CID, false, NULL, nsClipboardHelperConstructor },
    { &kNS_DRAGSERVICE_CID, false, NULL, nsDragServiceConstructor },
    { &kNS_HTMLFORMATCONVERTER_CID, false, NULL, nsHTMLFormatConverterConstructor },
    { &kNS_BIDIKEYBOARD_CID, false, NULL, nsBidiKeyboardConstructor },
    { &kNS_SCREENMANAGER_CID, false, NULL, nsScreenManagerQtConstructor },
    { &kNS_THEMERENDERER_CID, false, NULL, nsNativeThemeQtConstructor },
    { &kNS_IDLE_SERVICE_CID, false, NULL, nsIdleServiceQtConstructor },
    { &kNS_POPUP_CID, false, NULL, nsPopupWindowConstructor },
    { &kNS_TOOLKIT_CID, false, NULL, nsToolkitConstructor },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kWidgetContracts[] = {
    { "@mozilla.org/widget/window/qt;1", &kNS_WINDOW_CID },
    { "@mozilla.org/widgets/child_window/qt;1", &kNS_CHILD_CID },
    { "@mozilla.org/widget/appshell/qt;1", &kNS_APPSHELL_CID },
    { "@mozilla.org/widget/lookandfeel;1", &kNS_LOOKANDFEEL_CID },
    { "@mozilla.org/filepicker;1", &kNS_FILEPICKER_CID },
    { "@mozilla.org/sound;1", &kNS_SOUND_CID },
    { "@mozilla.org/widget/transferable;1", &kNS_TRANSFERABLE_CID },
    { "@mozilla.org/widget/clipboard;1", &kNS_CLIPBOARD_CID },
    { "@mozilla.org/widget/clipboardhelper;1", &kNS_CLIPBOARDHELPER_CID },
    { "@mozilla.org/widget/dragservice;1", &kNS_DRAGSERVICE_CID },
    { "@mozilla.org/widget/htmlformatconverter;1", &kNS_HTMLFORMATCONVERTER_CID },
    { "@mozilla.org/widget/bidikeyboard;1", &kNS_BIDIKEYBOARD_CID },
    { "@mozilla.org/gfx/screenmanager;1", &kNS_SCREENMANAGER_CID },
    { "@mozilla.org/chrome/chrome-native-theme;1", &kNS_THEMERENDERER_CID },
    { "@mozilla.org/widget/idleservice;1", &kNS_IDLE_SERVICE_CID },
    { "@mozilla.org/widgets/popup_window/qt;1", &kNS_POPUP_CID },
    { "@mozilla.org/widget/toolkit/qt;1", &kNS_TOOLKIT_CID },
    { NULL }
};

static void
nsWidgetQtModuleDtor()
{
    nsSound::Shutdown();
    nsWindow::ReleaseGlobals();
    nsAppShellShutdown();
}

static const mozilla::Module kWidgetModule = {
    mozilla::Module::kVersion,
    kWidgetCIDs,
    kWidgetContracts,
    NULL,
    NULL,
    nsAppShellInit,
    nsWidgetQtModuleDtor
};

NSMODULE_DEFN(nsWidgetQtModule) = &kWidgetModule;
