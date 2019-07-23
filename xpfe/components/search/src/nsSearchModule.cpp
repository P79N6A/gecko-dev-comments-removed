





































#ifndef MOZ_PHOENIX
#include "nsInternetSearchService.h"
#endif
#include "nsLocalSearchService.h"
#include "nsIGenericFactory.h"
#include "nsRDFCID.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(LocalSearchDataSource, Init)
#ifndef MOZ_PHOENIX
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(InternetSearchDataSource, Init)
#endif

static const nsModuleComponentInfo components[] = {
    { "Local Search", NS_RDFFINDDATASOURCE_CID,
      NS_LOCALSEARCH_SERVICE_CONTRACTID, LocalSearchDataSourceConstructor },
    { "Local Search", NS_RDFFINDDATASOURCE_CID,
      NS_LOCALSEARCH_DATASOURCE_CONTRACTID, LocalSearchDataSourceConstructor },
#ifndef MOZ_PHOENIX
    { "Internet Search", NS_RDFSEARCHDATASOURCE_CID,
      NS_INTERNETSEARCH_SERVICE_CONTRACTID, InternetSearchDataSourceConstructor },
    { "Internet Search", NS_RDFSEARCHDATASOURCE_CID,
      NS_INTERNETSEARCH_DATASOURCE_CONTRACTID, InternetSearchDataSourceConstructor },
#endif
};

NS_IMPL_NSGETMODULE(SearchServiceModule, components)
