




#include "nsCOMPtr.h"
#include "mozilla/ModuleUtils.h"

#include "mozStorageService.h"
#include "mozStorageConnection.h"
#include "VacuumManager.h"

#include "mozStorageCID.h"

namespace mozilla {
namespace storage {

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(Service,
                                         Service::getSingleton)
NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(VacuumManager,
                                         VacuumManager::getSingleton)

} 
} 

NS_DEFINE_NAMED_CID(MOZ_STORAGE_SERVICE_CID);
NS_DEFINE_NAMED_CID(VACUUMMANAGER_CID);

static const mozilla::Module::CIDEntry kStorageCIDs[] = {
    { &kMOZ_STORAGE_SERVICE_CID, false, nullptr, mozilla::storage::ServiceConstructor },
    { &kVACUUMMANAGER_CID, false, nullptr, mozilla::storage::VacuumManagerConstructor },
    { nullptr }
};

static const mozilla::Module::ContractIDEntry kStorageContracts[] = {
    { MOZ_STORAGE_SERVICE_CONTRACTID, &kMOZ_STORAGE_SERVICE_CID },
    { VACUUMMANAGER_CONTRACTID, &kVACUUMMANAGER_CID },
    { nullptr }
};

static const mozilla::Module::CategoryEntry kStorageCategories[] = {
    { "idle-daily", "MozStorage Vacuum Manager", VACUUMMANAGER_CONTRACTID },
    { nullptr }
};

static const mozilla::Module kStorageModule = {
    mozilla::Module::kVersion,
    kStorageCIDs,
    kStorageContracts,
    kStorageCategories
};

NSMODULE_DEFN(mozStorageModule) = &kStorageModule;
