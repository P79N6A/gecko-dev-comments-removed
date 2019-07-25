




#include "mozilla/ModuleUtils.h"

#include "nsMediaSniffer.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsMediaSniffer)

NS_DEFINE_NAMED_CID(NS_MEDIA_SNIFFER_CID);

static const mozilla::Module::CIDEntry kMediaSnifferCIDs[] = {
    { &kNS_MEDIA_SNIFFER_CID, false, NULL, nsMediaSnifferConstructor },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kMediaSnifferContracts[] = {
    { NS_MEDIA_SNIFFER_CONTRACTID, &kNS_MEDIA_SNIFFER_CID },
    { NULL }
};

static const mozilla::Module::CategoryEntry kMediaSnifferCategories[] = {
    { "content-sniffing-services", NS_MEDIA_SNIFFER_CONTRACTID, NS_MEDIA_SNIFFER_CONTRACTID},
    { "net-content-sniffers", NS_MEDIA_SNIFFER_CONTRACTID, NS_MEDIA_SNIFFER_CONTRACTID},
    { NULL }
};

static const mozilla::Module kMediaSnifferModule = {
    mozilla::Module::kVersion,
    kMediaSnifferCIDs,
    kMediaSnifferContracts,
    kMediaSnifferCategories
};

NSMODULE_DEFN(nsMediaSnifferModule) = &kMediaSnifferModule;
