





































#include "nsIGenericFactory.h"
#include "nsWidgetsCID.h"
#include "nsAppShell.h"
#include "nsAppShellSingleton.h"
#include "nsBaseWidget.h"
#include "nsLookAndFeel.h"
#include "nsWindow.h"
#include "nsTransferable.h"
#include "nsHTMLFormatConverter.h"
#ifdef MOZ_X11
#include "nsClipboardHelper.h"
#include "nsClipboard.h"
#include "nsDragService.h"
#endif
#include "nsFilePicker.h"
#include "nsSound.h"
#include "nsBidiKeyboard.h"
#include "nsNativeKeyBindings.h"
#include "nsScreenManagerGtk.h"
#include "nsAccelerometerUnix.h"

#ifdef NS_PRINTING
#include "nsPrintOptionsGTK.h"
#include "nsPrintSession.h"
#include "nsDeviceContextSpecG.h"
#endif

#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsImageToPixbuf.h"
#include "nsPrintDialogGTK.h"

#if defined(MOZ_X11)
#include "nsIdleServiceGTK.h"
#endif

#ifdef NATIVE_THEME_SUPPORT
#include "nsNativeThemeGTK.h"
#endif

#include "nsIComponentRegistrar.h"
#include "nsComponentManagerUtils.h"
#include "nsAutoPtr.h"
#include <gtk/gtk.h>


#define XULFILEPICKER_CID \
  { 0x54ae32f8, 0x1dd2, 0x11b2, \
    { 0xa2, 0x09, 0xdf, 0x7c, 0x50, 0x53, 0x70, 0xf8} }
static NS_DEFINE_CID(kXULFilePickerCID, XULFILEPICKER_CID);
static NS_DEFINE_CID(kNativeFilePickerCID, NS_FILEPICKER_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsChildWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLookAndFeel)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTransferable)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBidiKeyboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsHTMLFormatConverter)
#ifdef MOZ_X11
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboardHelper)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsClipboard, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDragService)
#endif
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSound)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsScreenManagerGtk)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsImageToPixbuf)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAccelerometerUnix)


#ifdef NATIVE_THEME_SUPPORT

extern PRBool gDisableNativeTheme;

static NS_IMETHODIMP
nsNativeThemeGTKConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult)
{
    nsresult rv;
    nsNativeThemeGTK * inst;

    if (gDisableNativeTheme)
        return NS_ERROR_NO_INTERFACE;

    *aResult = NULL;
    if (NULL != aOuter) {
        rv = NS_ERROR_NO_AGGREGATION;
        return rv;
    }

    NS_NEWXPCOM(inst, nsNativeThemeGTK);
    if (NULL == inst) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        return rv;
    }
    NS_ADDREF(inst);
    rv = inst->QueryInterface(aIID, aResult);
    NS_RELEASE(inst);

    return rv;
}
#endif

#if defined(MOZ_X11)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsIdleServiceGTK)
#endif

#ifdef NS_PRINTING
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextSpecGTK)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrintOptionsGTK, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsPrinterEnumeratorGTK)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrintSession, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrintDialogServiceGTK, Init)
#endif

static NS_IMETHODIMP
nsFilePickerConstructor(nsISupports *aOuter, REFNSIID aIID,
                        void **aResult)
{
  *aResult = nsnull;
  if (aOuter != nsnull) {
    return NS_ERROR_NO_AGGREGATION;
  }

  PRBool allowPlatformPicker = PR_TRUE;
  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefs) {
    PRBool prefAllow;
    nsresult rv = prefs->GetBoolPref("ui.allow_platform_file_picker",
                                     &prefAllow);
    if (NS_SUCCEEDED(rv)) {
        allowPlatformPicker = prefAllow;
    }
  }
  
  nsCOMPtr<nsIFilePicker> picker;
  if (allowPlatformPicker && gtk_check_version(2,6,3) == NULL) {
      picker = new nsFilePicker;
  } else {
    picker = do_CreateInstance(kXULFilePickerCID);
  }

  if (!picker) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return picker->QueryInterface(aIID, aResult);
}

static NS_IMETHODIMP
nsNativeKeyBindingsConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult,
                               NativeKeyBindingsType aKeyBindingsType)
{
    nsresult rv;

    nsNativeKeyBindings *inst;

    *aResult = NULL;
    if (NULL != aOuter) {
        rv = NS_ERROR_NO_AGGREGATION;
        return rv;
    }

    NS_NEWXPCOM(inst, nsNativeKeyBindings);
    if (NULL == inst) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        return rv;
    }
    NS_ADDREF(inst);
    inst->Init(aKeyBindingsType);
    rv = inst->QueryInterface(aIID, aResult);
    NS_RELEASE(inst);

    return rv;
}

static NS_IMETHODIMP
nsNativeKeyBindingsInputConstructor(nsISupports *aOuter, REFNSIID aIID,
                                    void **aResult)
{
    return nsNativeKeyBindingsConstructor(aOuter, aIID, aResult,
                                          eKeyBindings_Input);
}

static NS_IMETHODIMP
nsNativeKeyBindingsTextAreaConstructor(nsISupports *aOuter, REFNSIID aIID,
                                       void **aResult)
{
    return nsNativeKeyBindingsConstructor(aOuter, aIID, aResult,
                                          eKeyBindings_TextArea);
}

static const nsModuleComponentInfo components[] =
{
    { "Gtk2 Window",
      NS_WINDOW_CID,
      "@mozilla.org/widget/window/gtk;1",
      nsWindowConstructor },
    { "Gtk2 Child Window",
      NS_CHILD_CID,
      "@mozilla.org/widgets/child_window/gtk;1",
      nsChildWindowConstructor },
    { "Gtk2 AppShell",
      NS_APPSHELL_CID,
      "@mozilla.org/widget/appshell/gtk;1",
      nsAppShellConstructor },
    { "Gtk2 Look And Feel",
      NS_LOOKANDFEEL_CID,
      "@mozilla.org/widget/lookandfeel;1",
      nsLookAndFeelConstructor },
    { "Gtk2 File Picker",
      NS_FILEPICKER_CID,
      "@mozilla.org/filepicker;1",
      nsFilePickerConstructor },
    { "Gtk2 Sound",
      NS_SOUND_CID,
      "@mozilla.org/sound;1",
      nsSoundConstructor },
    { "Accelerometer",
       NS_ACCELEROMETER_CID,
       NS_ACCELEROMETER_CONTRACTID,
       nsAccelerometerUnixConstructor },
    { "Transferable",
      NS_TRANSFERABLE_CID,
      "@mozilla.org/widget/transferable;1",
      nsTransferableConstructor },

#ifdef MOZ_X11
  { "Gtk Clipboard",
    NS_CLIPBOARD_CID,
    "@mozilla.org/widget/clipboard;1",
    nsClipboardConstructor },
  { "Clipboard Helper",
    NS_CLIPBOARDHELPER_CID,
    "@mozilla.org/widget/clipboardhelper;1",
    nsClipboardHelperConstructor },
  { "Gtk Drag Service",
    NS_DRAGSERVICE_CID,
    "@mozilla.org/widget/dragservice;1",
    nsDragServiceConstructor },
#endif
  { "HTML Format Converter",
    NS_HTMLFORMATCONVERTER_CID,
    "@mozilla.org/widget/htmlformatconverter;1",
    nsHTMLFormatConverterConstructor },
  { "Gtk2 Bidi Keyboard",
    NS_BIDIKEYBOARD_CID,
    "@mozilla.org/widget/bidikeyboard;1",
    nsBidiKeyboardConstructor },
  { "Input Native Keybindings",
    NS_NATIVEKEYBINDINGSINPUT_CID,
    NS_NATIVEKEYBINDINGSINPUT_CONTRACTID,
    nsNativeKeyBindingsInputConstructor },
  { "TextArea Native Keybindings",
    NS_NATIVEKEYBINDINGSTEXTAREA_CID,
    NS_NATIVEKEYBINDINGSTEXTAREA_CONTRACTID,
    nsNativeKeyBindingsTextAreaConstructor },
  { "Editor Native Keybindings",
    NS_NATIVEKEYBINDINGSEDITOR_CID,
    NS_NATIVEKEYBINDINGSEDITOR_CONTRACTID,
    nsNativeKeyBindingsTextAreaConstructor },
  { "Gtk Screen Manager",
    NS_SCREENMANAGER_CID,
    "@mozilla.org/gfx/screenmanager;1",
    nsScreenManagerGtkConstructor },
#ifdef NATIVE_THEME_SUPPORT
   { "Native Theme Renderer",
    NS_THEMERENDERER_CID,
    "@mozilla.org/chrome/chrome-native-theme;1",
     nsNativeThemeGTKConstructor },
#endif
#ifdef NS_PRINTING
  { "PrintSettings Service",
    NS_PRINTSETTINGSSERVICE_CID,
    "@mozilla.org/gfx/printsettings-service;1",
    nsPrintOptionsGTKConstructor },
  { "Gtk Printer Enumerator",
    NS_PRINTER_ENUMERATOR_CID,
    
    "@mozilla.org/gfx/printerenumerator;1",
    nsPrinterEnumeratorGTKConstructor },
  { "Print Session",
    NS_PRINTSESSION_CID,
    "@mozilla.org/gfx/printsession;1",
    nsPrintSessionConstructor },
  { "Gtk Device Context Spec",
    NS_DEVICE_CONTEXT_SPEC_CID,
    
    "@mozilla.org/gfx/devicecontextspec;1",
    nsDeviceContextSpecGTKConstructor },
  { "Native Print Dialog",
    NS_PRINTDIALOGSERVICE_CID,
    NS_PRINTDIALOGSERVICE_CONTRACTID,
    nsPrintDialogServiceGTKConstructor },
#endif 
  { "Image to gdk-pixbuf converter",
    NS_IMAGE_TO_PIXBUF_CID,
    "@mozilla.org/widget/image-to-gdk-pixbuf;1",
    nsImageToPixbufConstructor },
#if defined(MOZ_X11)
{ "User Idle Service",
    NS_IDLE_SERVICE_CID,
    "@mozilla.org/widget/idleservice;1",
    nsIdleServiceGTKConstructor },
#endif

};

static void
nsWidgetGtk2ModuleDtor(nsIModule *aSelf)
{
  nsFilePicker::Shutdown();
  nsSound::Shutdown();
  nsWindow::ReleaseGlobals();
  nsAppShellShutdown(aSelf);
}

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(nsWidgetGtk2Module,
                                   components,
                                   nsAppShellInit,
                                   nsWidgetGtk2ModuleDtor)
