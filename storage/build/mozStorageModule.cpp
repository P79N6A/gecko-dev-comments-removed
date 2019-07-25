






































#include "nsCOMPtr.h"
#include "mozilla/ModuleUtils.h"

#include "mozStorageService.h"
#include "mozStorageConnection.h"
#include "mozStorageStatementWrapper.h"

#include "mozStorageCID.h"

namespace mozilla {
namespace storage {

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(Service,
                                         Service::getSingleton)
NS_GENERIC_FACTORY_CONSTRUCTOR(StatementWrapper)


} 
} 

NS_DEFINE_NAMED_CID(MOZ_STORAGE_SERVICE_CID);
NS_DEFINE_NAMED_CID(MOZ_STORAGE_STATEMENT_WRAPPER_CID);

static const mozilla::Module::CIDEntry kStorageCIDs[] = {
    { &kMOZ_STORAGE_SERVICE_CID, false, NULL, mozilla::storage::ServiceConstructor },
    { &kMOZ_STORAGE_STATEMENT_WRAPPER_CID, false, NULL, mozilla::storage::StatementWrapperConstructor },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kStorageContracts[] = {
    { MOZ_STORAGE_SERVICE_CONTRACTID, &kMOZ_STORAGE_SERVICE_CID },
    { MOZ_STORAGE_STATEMENT_WRAPPER_CONTRACTID, &kMOZ_STORAGE_STATEMENT_WRAPPER_CID },
    { NULL }
};

static const mozilla::Module kStorageModule = {
    mozilla::Module::kVersion,
    kStorageCIDs,
    kStorageContracts
};

NSMODULE_DEFN(mozStorageModule) = &kStorageModule;
