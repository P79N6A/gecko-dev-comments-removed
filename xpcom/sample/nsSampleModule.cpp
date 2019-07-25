





































#include "mozilla/ModuleUtils.h"
#include "nsIClassInfoImpl.h"

#include "nsSample.h"


















NS_GENERIC_FACTORY_CONSTRUCTOR(nsSampleImpl)


NS_DEFINE_NAMED_CID(NS_SAMPLE_CID);





static const mozilla::Module::CIDEntry kSampleCIDs[] = {
    { &kNS_SAMPLE_CID, false, NULL, nsSampleImplConstructor },
    { NULL }
};





static const mozilla::Module::ContractIDEntry kSampleContracts[] = {
    { NS_SAMPLE_CONTRACTID, &kNS_SAMPLE_CID },
    { NULL }
};






static const mozilla::Module::CategoryEntry kSampleCategories[] = {
    { "my-category", "my-key", NS_SAMPLE_CONTRACTID },
    { NULL }
};

static const mozilla::Module kSampleModule = {
    mozilla::Module::kVersion,
    kSampleCIDs,
    kSampleContracts,
    kSampleCategories
};



NSMODULE_DEFN(nsSampleModule) = &kSampleModule;
