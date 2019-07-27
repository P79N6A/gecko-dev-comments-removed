






#include "mozilla/ModuleUtils.h"
#include "nsWidgetsCID.h"
#include "nsClipboardProxy.h"
#include "nsColorPickerProxy.h"
#include "nsFilePickerProxy.h"
#include "nsScreenManagerProxy.h"
#include "PuppetBidiKeyboard.h"

using namespace mozilla;
using namespace mozilla::widget;

#ifndef MOZ_B2G

NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboardProxy)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsColorPickerProxy)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFilePickerProxy)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsScreenManagerProxy)
NS_GENERIC_FACTORY_CONSTRUCTOR(PuppetBidiKeyboard)

NS_DEFINE_NAMED_CID(NS_CLIPBOARD_CID);
NS_DEFINE_NAMED_CID(NS_COLORPICKER_CID);
NS_DEFINE_NAMED_CID(NS_FILEPICKER_CID);
NS_DEFINE_NAMED_CID(NS_SCREENMANAGER_CID);
NS_DEFINE_NAMED_CID(PUPPETBIDIKEYBOARD_CID);

static const mozilla::Module::CIDEntry kWidgetCIDs[] = {
    { &kNS_CLIPBOARD_CID, false, nullptr, nsClipboardProxyConstructor,
      Module::CONTENT_PROCESS_ONLY },
    { &kNS_COLORPICKER_CID, false, nullptr, nsColorPickerProxyConstructor,
      Module::CONTENT_PROCESS_ONLY },
    { &kNS_FILEPICKER_CID, false, nullptr, nsFilePickerProxyConstructor,
      Module::CONTENT_PROCESS_ONLY },
    { &kNS_SCREENMANAGER_CID, false, nullptr, nsScreenManagerProxyConstructor,
      Module::CONTENT_PROCESS_ONLY },
    { &kPUPPETBIDIKEYBOARD_CID, false, NULL, PuppetBidiKeyboardConstructor,
      mozilla::Module::CONTENT_PROCESS_ONLY },
    { nullptr }
};

static const mozilla::Module::ContractIDEntry kWidgetContracts[] = {
    { "@mozilla.org/widget/clipboard;1", &kNS_CLIPBOARD_CID, Module::CONTENT_PROCESS_ONLY },
    { "@mozilla.org/colorpicker;1", &kNS_COLORPICKER_CID, Module::CONTENT_PROCESS_ONLY },
    { "@mozilla.org/filepicker;1", &kNS_FILEPICKER_CID, Module::CONTENT_PROCESS_ONLY },
    { "@mozilla.org/gfx/screenmanager;1", &kNS_SCREENMANAGER_CID, Module::CONTENT_PROCESS_ONLY },
    { "@mozilla.org/widget/bidikeyboard;1", &kPUPPETBIDIKEYBOARD_CID, Module::CONTENT_PROCESS_ONLY },
    { nullptr }
};

static const mozilla::Module kWidgetModule = {
    mozilla::Module::kVersion,
    kWidgetCIDs,
    kWidgetContracts
};

NSMODULE_DEFN(nsContentProcessWidgetModule) = &kWidgetModule;

#endif 
