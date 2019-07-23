






































#include "nsCOMPtr.h"
#include "nsIGenericFactory.h"
#include "nsIModule.h"

#include "mozStorageService.h"
#include "mozStorageConnection.h"
#include "mozStorageStatementWrapper.h"

#include "mozStorageCID.h"

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(mozStorageService,
                                         mozStorageService::GetSingleton)
NS_GENERIC_FACTORY_CONSTRUCTOR(mozStorageStatementWrapper)

static const nsModuleComponentInfo components[] =
{
    { "Unified Data Store Service",
      MOZ_STORAGE_SERVICE_CID,
      MOZ_STORAGE_SERVICE_CONTRACTID,
      mozStorageServiceConstructor
    },

    { "Unified Data Store Scriptable Statement Wrapper",
      MOZ_STORAGE_STATEMENT_WRAPPER_CID,
      MOZ_STORAGE_STATEMENT_WRAPPER_CONTRACTID,
      mozStorageStatementWrapperConstructor
    }
};

NS_IMPL_NSGETMODULE(mozStorageModule, components)
