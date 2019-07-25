








































#include "nsWindow.h"
#include "nsAppShell.h"

#include "nsIGenericFactory.h"
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

static NS_IMETHODIMP
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

static const nsModuleComponentInfo components[] =
{
    { "Qt nsWindow",
      NS_WINDOW_CID,
      "@mozilla.org/widgets/window/qt;1",
      nsWindowConstructor },
    { "Qt Child nsWindow",
      NS_CHILD_CID,
      "@mozilla.org/widgets/child_window/qt;1",
      nsChildWindowConstructor },
    { "Qt AppShell",
      NS_APPSHELL_CID,
      "@mozilla.org/widget/appshell/qt;1",
      nsAppShellConstructor },
    { "Qt Look And Feel",
      NS_LOOKANDFEEL_CID,
      "@mozilla.org/widget/lookandfeel/qt;1",
      nsLookAndFeelConstructor },
    { "Qt Popup nsWindow",
      NS_POPUP_CID,
      "@mozilla.org/widgets/popup_window/qt;1",
      nsPopupWindowConstructor },
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
    { "Qt Screen Manager",
      NS_SCREENMANAGER_CID,
      "@mozilla.org/gfx/screenmanager;1",
      nsScreenManagerQtConstructor },
    { "Qt Clipboard",
      NS_CLIPBOARD_CID,
      "@mozilla.org/widget/clipboard;1",
      nsClipboardConstructor },
    { "Clipboard Helper",
      NS_CLIPBOARDHELPER_CID,
      "@mozilla.org/widget/clipboardhelper;1",
      nsClipboardHelperConstructor },
    { "Qt Drag Service",
      NS_DRAGSERVICE_CID,
      "@mozilla.org/widget/dragservice;1",
      nsDragServiceConstructor },
    { "Qt Bidi Keyboard",
      NS_BIDIKEYBOARD_CID,
      "@mozilla.org/widget/bidikeyboard;1",
      nsBidiKeyboardConstructor },
    { "Qt Idle Service",
       NS_IDLE_SERVICE_CID,
       "@mozilla.org/widget/idleservice;1",
       nsIdleServiceQtConstructor },
    { "Qt Sound",
      NS_SOUND_CID,
      "@mozilla.org/sound;1",
      nsSoundConstructor },
    { "Native Theme Renderer",
      NS_THEMERENDERER_CID,
      "@mozilla.org/chrome/chrome-native-theme;1",
      nsNativeThemeQtConstructor },
    { "Qt File Picker",
      NS_FILEPICKER_CID,
      "@mozilla.org/filepicker;1",
      nsFilePickerConstructor }

};

static void
nsWidgetQtModuleDtor(nsIModule *aSelf)
{
    nsSound::Shutdown();
    nsAppShellShutdown(aSelf);
}

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(nsWidgetQtModule,
                                   components,
                                   nsAppShellInit,
                                   nsWidgetQtModuleDtor)

