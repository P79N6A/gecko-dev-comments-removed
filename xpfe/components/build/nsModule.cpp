




































#include "mozilla/ModuleUtils.h"
#include "nsNetUtil.h"
#include "nsDirectoryViewer.h"
#include "rdf.h"
#include "nsRDFCID.h"
#include "nsCURILoader.h"


NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsHTTPIndex, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDirectoryViewerFactory)

NS_DEFINE_NAMED_CID(NS_DIRECTORYVIEWERFACTORY_CID);
NS_DEFINE_NAMED_CID(NS_HTTPINDEX_SERVICE_CID);

static const mozilla::Module::CIDEntry kXPFECIDs[] = {
    { &kNS_DIRECTORYVIEWERFACTORY_CID, false, NULL, nsDirectoryViewerFactoryConstructor },
    { &kNS_HTTPINDEX_SERVICE_CID, false, NULL, nsHTTPIndexConstructor },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kXPFEContracts[] = {
    { "@mozilla.org/xpfe/http-index-format-factory-constructor", &kNS_DIRECTORYVIEWERFACTORY_CID },
    { NS_HTTPINDEX_SERVICE_CONTRACTID, &kNS_HTTPINDEX_SERVICE_CID },
    { NS_HTTPINDEX_DATASOURCE_CONTRACTID, &kNS_HTTPINDEX_SERVICE_CID },
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
