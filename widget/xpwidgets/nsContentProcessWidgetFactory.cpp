






#include "mozilla/ModuleUtils.h"
#include "nsWidgetsCID.h"
#include "nsFilePickerProxy.h"

using namespace mozilla;

#ifndef MOZ_B2G

NS_GENERIC_FACTORY_CONSTRUCTOR(nsFilePickerProxy)

NS_DEFINE_NAMED_CID(NS_FILEPICKER_CID);

static const mozilla::Module::CIDEntry kWidgetCIDs[] = {
    { &kNS_FILEPICKER_CID, false, nullptr, nsFilePickerProxyConstructor,
      Module::CONTENT_PROCESS_ONLY },
    { nullptr }
};

static const mozilla::Module::ContractIDEntry kWidgetContracts[] = {
    { "@mozilla.org/filepicker;1", &kNS_FILEPICKER_CID, Module::CONTENT_PROCESS_ONLY },
    { nullptr }
};

static const mozilla::Module kWidgetModule = {
    mozilla::Module::kVersion,
    kWidgetCIDs,
    kWidgetContracts
};

NSMODULE_DEFN(nsContentProcessWidgetModule) = &kWidgetModule;

#endif 
