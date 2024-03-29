















#include "base/basictypes.h"

#include "mozilla/ModuleUtils.h"
#include "mozilla/WidgetUtils.h"

#include "nsCOMPtr.h"
#include "nsWidgetsCID.h"
#include "nsAppShell.h"

#include "nsWindow.h"
#include "nsLookAndFeel.h"
#include "nsAppShellSingleton.h"
#include "nsScreenManagerGonk.h"
#include "nsIdleServiceGonk.h"
#include "nsTransferable.h"
#include "nsClipboard.h"
#include "nsClipboardHelper.h"

#include "nsHTMLFormatConverter.h"
#include "nsXULAppAPI.h"

#include "PuppetWidget.h"

using namespace mozilla::widget;



#include "GfxInfo.h"
namespace mozilla {
namespace widget {

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(GfxInfo, Init)
}
}

NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsScreenManagerGonk)
NS_GENERIC_FACTORY_CONSTRUCTOR(PuppetScreenManager)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsHTMLFormatConverter)
NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsIdleServiceGonk, nsIdleServiceGonk::GetInstance)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTransferable)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboardHelper)

NS_DEFINE_NAMED_CID(NS_APPSHELL_CID);
NS_DEFINE_NAMED_CID(NS_WINDOW_CID);
NS_DEFINE_NAMED_CID(NS_CHILD_CID);
NS_DEFINE_NAMED_CID(NS_SCREENMANAGER_CID);
NS_DEFINE_NAMED_CID(NS_HTMLFORMATCONVERTER_CID);
NS_DEFINE_NAMED_CID(NS_IDLE_SERVICE_CID);
NS_DEFINE_NAMED_CID(NS_TRANSFERABLE_CID);
NS_DEFINE_NAMED_CID(NS_GFXINFO_CID);
NS_DEFINE_NAMED_CID(NS_CLIPBOARD_CID);
NS_DEFINE_NAMED_CID(NS_CLIPBOARDHELPER_CID);

static nsresult
ScreenManagerConstructor(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    return (XRE_IsParentProcess()) ?
        nsScreenManagerGonkConstructor(aOuter, aIID, aResult) :
        PuppetScreenManagerConstructor(aOuter, aIID, aResult);
}

static const mozilla::Module::CIDEntry kWidgetCIDs[] = {
    { &kNS_WINDOW_CID, false, nullptr, nsWindowConstructor },
    { &kNS_CHILD_CID, false, nullptr, nsWindowConstructor },
    { &kNS_APPSHELL_CID, false, nullptr, nsAppShellConstructor },
    { &kNS_SCREENMANAGER_CID, false, nullptr, ScreenManagerConstructor },
    { &kNS_HTMLFORMATCONVERTER_CID, false, nullptr, nsHTMLFormatConverterConstructor },
    { &kNS_IDLE_SERVICE_CID, false, nullptr, nsIdleServiceGonkConstructor },
    { &kNS_TRANSFERABLE_CID, false, nullptr, nsTransferableConstructor },
    { &kNS_GFXINFO_CID, false, nullptr, mozilla::widget::GfxInfoConstructor },
    { &kNS_CLIPBOARD_CID, false, nullptr, nsClipboardConstructor },
    { &kNS_CLIPBOARDHELPER_CID, false, nullptr, nsClipboardHelperConstructor },
    { nullptr }
};

static const mozilla::Module::ContractIDEntry kWidgetContracts[] = {
    { "@mozilla.org/widgets/window/gonk;1", &kNS_WINDOW_CID },
    { "@mozilla.org/widgets/child_window/gonk;1", &kNS_CHILD_CID },
    { "@mozilla.org/widget/appshell/gonk;1", &kNS_APPSHELL_CID },
    { "@mozilla.org/gfx/screenmanager;1", &kNS_SCREENMANAGER_CID },
    { "@mozilla.org/widget/htmlformatconverter;1", &kNS_HTMLFORMATCONVERTER_CID },
    { "@mozilla.org/widget/idleservice;1", &kNS_IDLE_SERVICE_CID },
    { "@mozilla.org/widget/transferable;1", &kNS_TRANSFERABLE_CID },
    { "@mozilla.org/gfx/info;1", &kNS_GFXINFO_CID },
    { "@mozilla.org/widget/clipboard;1", &kNS_CLIPBOARD_CID },
    { "@mozilla.org/widget/clipboardhelper;1", &kNS_CLIPBOARDHELPER_CID },
    { nullptr }
};

static void
nsWidgetGonkModuleDtor()
{
    
    WidgetUtils::Shutdown();

    nsLookAndFeel::Shutdown();
    nsAppShellShutdown();
}

static const mozilla::Module kWidgetModule = {
    mozilla::Module::kVersion,
    kWidgetCIDs,
    kWidgetContracts,
    nullptr,
    nullptr,
    nsAppShellInit,
    nsWidgetGonkModuleDtor
};

NSMODULE_DEFN(nsWidgetGonkModule) = &kWidgetModule;
