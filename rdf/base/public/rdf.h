











































#ifndef rdf_h___
#define rdf_h___

#include "nsError.h"














#define DEFINE_RDF_VOCAB(ns, prefix, name) \
static const char kURI##prefix##_##name[] = ns #name





#define RDF_NAMESPACE_URI  "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define WEB_NAMESPACE_URI  "http://home.netscape.com/WEB-rdf#"
#define NC_NAMESPACE_URI   "http://home.netscape.com/NC-rdf#"
#define DEVMO_NAMESPACE_URI_PREFIX "http://developer.mozilla.org/rdf/vocabulary/"









#define NS_RDF_CURSOR_EMPTY       NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_RDF, 1)



#define NS_RDF_NO_VALUE           NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_RDF, 2)



#define NS_RDF_ASSERTION_ACCEPTED NS_OK




#define NS_RDF_ASSERTION_REJECTED NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_RDF, 3)


#define NS_RDF_STOP_VISIT         NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_RDF, 4)




#define NS_RDF_CONTRACTID                           "@mozilla.org/rdf"
#define NS_RDF_DATASOURCE_CONTRACTID                NS_RDF_CONTRACTID "/datasource;1"
#define NS_RDF_DATASOURCE_CONTRACTID_PREFIX         NS_RDF_DATASOURCE_CONTRACTID "?name="
#define NS_RDF_RESOURCE_FACTORY_CONTRACTID          "@mozilla.org/rdf/resource-factory;1"
#define NS_RDF_RESOURCE_FACTORY_CONTRACTID_PREFIX   NS_RDF_RESOURCE_FACTORY_CONTRACTID "?name="
#define NS_RDF_INFER_DATASOURCE_CONTRACTID_PREFIX   NS_RDF_CONTRACTID "/infer-datasource;1?engine="

#define NS_RDF_SERIALIZER                            NS_RDF_CONTRACTID "/serializer;1?format="



#define NS_RDF_DELEGATEFACTORY_CONTRACTID    "@mozilla.org/rdf/delegate-factory;1"
#define NS_RDF_DELEGATEFACTORY_CONTRACTID_PREFIX    NS_RDF_DELEGATEFACTORY_CONTRACTID "?key="



#endif 
