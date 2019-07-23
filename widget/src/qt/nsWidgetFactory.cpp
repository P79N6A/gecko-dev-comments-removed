








































#include "nsIGenericFactory.h"
#include "nsIModule.h"
#include "nsCOMPtr.h"
#include "nsWidgetsCID.h"
#include "nsIComponentRegistrar.h"
#include "nsComponentManagerUtils.h"
#include "nsAutoPtr.h"

#include "nsWindow.h"
#include "nsAppShell.h"
#include "nsToolkit.h"
#include "nsLookAndFeel.h"
#include "nsTransferable.h"
#include "nsClipboard.h"
#include "nsClipboardHelper.h"
#include "nsHTMLFormatConverter.h"
#include "nsDragService.h"
#include "nsScrollbar.h"
#include "nsFilePicker.h"
#include "nsSound.h"

#include "nsGUIEvent.h"
#include "nsQtEventDispatcher.h"
#include "nsIRenderingContext.h"
#include "nsIServiceManager.h"
#include "nsGfxCIID.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"

#include "nsBidiKeyboard.h"
#include "nsNativeThemeQt.h"

static NS_DEFINE_CID(kNativeScrollCID, NS_NATIVESCROLLBAR_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(ChildWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(PopupWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAppShell)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsToolkit)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLookAndFeel)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTransferable)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboardHelper)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsHTMLFormatConverter)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDragService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBidiKeyboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsNativeScrollbar)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSound)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFilePicker)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsNativeThemeQt)

static const nsModuleComponentInfo components[] =
{
    { "Qt nsWindow",
      NS_WINDOW_CID,
      "@mozilla.org/widgets/window/qt;1",
      nsWindowConstructor },
    { "Qt Child nsWindow",
      NS_CHILD_CID,
      "@mozilla.org/widgets/child_window/qt;1",
      ChildWindowConstructor },
    { "Qt Popup nsWindow",
      NS_POPUP_CID,
      "@mozilla.org/widgets/popup_window/qt;1",
      PopupWindowConstructor },
    { "Qt Native Scrollbar",
      NS_NATIVESCROLLBAR_CID,
      "@mozilla.org/widget/nativescrollbar/qt;1",
      nsNativeScrollbarConstructor},
    { "Qt AppShell",
      NS_APPSHELL_CID,
      "@mozilla.org/widget/appshell/qt;1",
      nsAppShellConstructor },
    { "Qt Toolkit",
      NS_TOOLKIT_CID,
      "@mozilla.org/widget/toolkit/qt;1",
      nsToolkitConstructor },
    { "Qt Look And Feel",
      NS_LOOKANDFEEL_CID,
      "@mozilla.org/widget/lookandfeel/qt;1",
      nsLookAndFeelConstructor },
    { "Transferrable",
      NS_TRANSFERABLE_CID,
      "@mozilla.org/widget/transferable;1",
      nsTransferableConstructor },
    { "Qt Clipboard",
      NS_CLIPBOARD_CID,
      "@mozilla.org/widget/clipboard;1",
      nsClipboardConstructor },
    { "Clipboard Helper",
      NS_CLIPBOARDHELPER_CID,
      "@mozilla.org/widget/clipboardhelper;1",
      nsClipboardHelperConstructor },
    { "HTML Format Converter",
      NS_HTMLFORMATCONVERTER_CID,
      "@mozilla.org/widget/htmlformatconverter/qt;1",
      nsHTMLFormatConverterConstructor },
    { "Qt Drag Service",
      NS_DRAGSERVICE_CID,
      "@mozilla.org/widget/dragservice;1",
      nsDragServiceConstructor },
    { "Qt Bidi Keyboard",
      NS_BIDIKEYBOARD_CID,
      "@mozilla.org/widget/bidikeyboard;1",
      nsBidiKeyboardConstructor },
    { "Qt Sound",
      NS_SOUND_CID,
      "@mozilla.org/sound;1",
      nsSoundConstructor },
    { "Qt File Picker",
      NS_FILEPICKER_CID,
      "@mozilla.org/filepicker;1",
      nsFilePickerConstructor },
    { "Native Theme Renderer",
      NS_THEMERENDERER_CID,
      "@mozilla.org/chrome/chrome-native-theme;1",
      nsNativeThemeQtConstructor }
};

NS_IMPL_NSGETMODULE(nsWidgetQtModule,components)
