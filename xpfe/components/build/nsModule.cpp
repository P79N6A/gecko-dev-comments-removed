




































#include "mozilla/ModuleUtils.h"
#include "nsNetUtil.h"
#include "nsDirectoryViewer.h"
#ifdef MOZ_RDF
#include "rdf.h"
#include "nsRDFCID.h"
#endif
#include "nsCURILoader.h"

#ifdef MOZ_RDF

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsHTTPIndex, Init)
#endif
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDirectoryViewerFactory)

NS_DEFINE_NAMED_CID(NS_DIRECTORYVIEWERFACTORY_CID);
#ifdef MOZ_RDF
NS_DEFINE_NAMED_CID(NS_HTTPINDEX_SERVICE_CID);
#endif


static const mozilla::Module::CIDEntry kXPFECIDs[] = {
    { &kNS_DIRECTORYVIEWERFACTORY_CID, false, NULL, nsDirectoryViewerFactoryConstructor },
#ifdef MOZ_RDF
    { &kNS_HTTPINDEX_SERVICE_CID, false, NULL, nsHTTPIndexConstructor },
#endif
    { NULL }
};

static const mozilla::Module::ContractIDEntry kXPFEContracts[] = {
    { "@mozilla.org/xpfe/http-index-format-factory-constructor", &kNS_DIRECTORYVIEWERFACTORY_CID },
#ifdef MOZ_RDF
    { NS_HTTPINDEX_SERVICE_CONTRACTID, &kNS_HTTPINDEX_SERVICE_CID },
    { NS_HTTPINDEX_DATASOURCE_CONTRACTID, &kNS_HTTPINDEX_SERVICE_CID },
#endif
    { NULL }
};

static const mozilla::Module::CategoryEntry kXPFECategories[] = {
    { "Gecko-Content-Viewers", "application/http-index-format", "@mozilla.org/xpfe/http-index-format-factory-constructor" },
    { NULL }
};

static const mozilla::Module kXPFEModule = {
    mozilla::Module::kVersion,
    kXPFECIDs,
    kXPFEContracts,
    kXPFECategories
};

NSMODULE_DEFN(application) = &kXPFEModule;
