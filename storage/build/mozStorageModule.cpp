






































#include "nsCOMPtr.h"
#include "nsIGenericFactory.h"
#include "nsIModule.h"

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

static const nsModuleComponentInfo components[] =
{
    { "Unified Data Store Service",
      MOZ_STORAGE_SERVICE_CID,
      MOZ_STORAGE_SERVICE_CONTRACTID,
      mozilla::storage::ServiceConstructor
    },

    { "Unified Data Store Scriptable Statement Wrapper",
      MOZ_STORAGE_STATEMENT_WRAPPER_CID,
      MOZ_STORAGE_STATEMENT_WRAPPER_CONTRACTID,
      mozilla::storage::StatementWrapperConstructor
    }
};

NS_IMPL_NSGETMODULE(mozStorageModule, components)
