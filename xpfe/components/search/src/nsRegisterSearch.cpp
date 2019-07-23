






































#include "nsRDFCID.h"
#include "nsIGenericFactory.h"
#include "nsISearchService.h"
#include "nsLocalSearchService.h"
#include "nsInternetSearchService.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(LocalSearchDataSource, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(InternetSearchDataSource, Init)

static const nsModuleComponentInfo components[] =
{
    { "Local Search", NS_RDFFINDDATASOURCE_CID,
      NS_LOCALSEARCH_SERVICE_CONTRACTID, LocalSearchDataSourceConstructor,
    },
    { "Local Search", NS_RDFFINDDATASOURCE_CID,
      NS_LOCALSEARCH_DATASOURCE_CONTRACTID, LocalSearchDataSourceConstructor,
    },
    { "Internet Search", NS_RDFSEARCHDATASOURCE_CID,
      NS_INTERNETSEARCH_SERVICE_CONTRACTID, InternetSearchDataSourceConstructor,
    },
    { "Internet Search", NS_RDFSEARCHDATASOURCE_CID,
      NS_INTERNETSEARCH_DATASOURCE_CONTRACTID, InternetSearchDataSourceConstructor,
    },
};

NS_IMPL_NSGETMODULE(nsSearchModule, components)

