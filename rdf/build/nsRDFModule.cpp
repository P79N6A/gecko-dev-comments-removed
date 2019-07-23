




































#include "nsCOMPtr.h"
#include "nsIGenericFactory.h"
#include "nsIModule.h"

#include "nsRDFModule.h"
#include "nsIFactory.h"
#include "nsRDFService.h"
#include "nsIRDFContainer.h"
#include "nsIRDFContainerUtils.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFContentSink.h"
#include "nsISupports.h"
#include "nsRDFBaseDataSources.h"
#include "nsRDFBuiltInDataSources.h"
#include "nsFileSystemDataSource.h"
#include "nsRDFCID.h"
#include "nsIComponentManager.h"
#include "rdf.h"
#include "nsIServiceManager.h"
#include "nsILocalStore.h"
#include "nsRDFXMLParser.h"
#include "nsRDFXMLSerializer.h"

#include "rdfISerializer.h"






#define MAKE_CTOR(_func,_new,_ifname)                                \
static NS_IMETHODIMP                                                 \
CreateNew##_func(nsISupports* aOuter, REFNSIID aIID, void **aResult) \
{                                                                    \
    if (!aResult) {                                                  \
        return NS_ERROR_INVALID_POINTER;                             \
    }                                                                \
    if (aOuter) {                                                    \
        *aResult = nsnull;                                           \
        return NS_ERROR_NO_AGGREGATION;                              \
    }                                                                \
    nsI##_ifname* inst;                                              \
    nsresult rv = NS_New##_new(&inst);                               \
    if (NS_FAILED(rv)) {                                             \
        *aResult = nsnull;                                           \
        return rv;                                                   \
    }                                                                \
    rv = inst->QueryInterface(aIID, aResult);                        \
    if (NS_FAILED(rv)) {                                             \
        *aResult = nsnull;                                           \
    }                                                                \
    NS_RELEASE(inst);             /* get rid of extra refcnt */      \
    return rv;                                                       \
}

extern nsresult
NS_NewDefaultResource(nsIRDFResource** aResult);

MAKE_CTOR(RDFXMLDataSource,RDFXMLDataSource,RDFDataSource)
MAKE_CTOR(RDFCompositeDataSource,RDFCompositeDataSource,RDFCompositeDataSource)
MAKE_CTOR(RDFContainer,RDFContainer,RDFContainer)

MAKE_CTOR(RDFContainerUtils,RDFContainerUtils,RDFContainerUtils)

MAKE_CTOR(RDFContentSink,RDFContentSink,RDFContentSink)
MAKE_CTOR(RDFDefaultResource,DefaultResource,RDFResource)

#define MAKE_RDF_CTOR(_func,_new,_ifname)                            \
static NS_IMETHODIMP                                                 \
CreateNew##_func(nsISupports* aOuter, REFNSIID aIID, void **aResult) \
{                                                                    \
    if (!aResult) {                                                  \
        return NS_ERROR_INVALID_POINTER;                             \
    }                                                                \
    if (aOuter) {                                                    \
        *aResult = nsnull;                                           \
        return NS_ERROR_NO_AGGREGATION;                              \
    }                                                                \
    rdfI##_ifname* inst;                                             \
    nsresult rv = NS_New##_new(&inst);                               \
    if (NS_FAILED(rv)) {                                             \
        *aResult = nsnull;                                           \
        return rv;                                                   \
    }                                                                \
    rv = inst->QueryInterface(aIID, aResult);                        \
    if (NS_FAILED(rv)) {                                             \
        *aResult = nsnull;                                           \
    }                                                                \
    NS_RELEASE(inst);             /* get rid of extra refcnt */      \
    return rv;                                                       \
}

extern nsresult
NS_NewTriplesSerializer(rdfISerializer** aResult);

MAKE_RDF_CTOR(TriplesSerializer, TriplesSerializer, Serializer)


static const nsModuleComponentInfo components[] = 
{   
    { 
     "RDF Composite Data Source", 
     NS_RDFCOMPOSITEDATASOURCE_CID,
     NS_RDF_DATASOURCE_CONTRACTID_PREFIX "composite-datasource", 
     CreateNewRDFCompositeDataSource
    },

    { 
     "RDF File System Data Source", 
     NS_RDFFILESYSTEMDATASOURCE_CID,
     NS_RDF_DATASOURCE_CONTRACTID_PREFIX "files", 
     FileSystemDataSource::Create
    },
    
    { "RDF In-Memory Data Source", 
      NS_RDFINMEMORYDATASOURCE_CID,
      NS_RDF_DATASOURCE_CONTRACTID_PREFIX "in-memory-datasource", 
      NS_NewRDFInMemoryDataSource
    },

    { "RDF XML Data Source", 
      NS_RDFXMLDATASOURCE_CID,
      NS_RDF_DATASOURCE_CONTRACTID_PREFIX "xml-datasource", 
      CreateNewRDFXMLDataSource
    },

    
    { "RDF Default Resource Factory", 
      NS_RDFDEFAULTRESOURCE_CID,
      
      NS_RDF_RESOURCE_FACTORY_CONTRACTID, 
      CreateNewRDFDefaultResource
    },

    
    { "RDF Content Sink", 
      NS_RDFCONTENTSINK_CID,
      NS_RDF_CONTRACTID "/content-sink;1", 
      CreateNewRDFContentSink
    },
    
    { "RDF Container", 
      NS_RDFCONTAINER_CID,
      NS_RDF_CONTRACTID "/container;1", 
      CreateNewRDFContainer
    },
    
    { "RDF Container Utilities", 
      NS_RDFCONTAINERUTILS_CID,
      NS_RDF_CONTRACTID "/container-utils;1", 
      CreateNewRDFContainerUtils
    },
    
    { "RDF Service", 
      NS_RDFSERVICE_CID,
      NS_RDF_CONTRACTID "/rdf-service;1",
      RDFServiceImpl::CreateSingleton
    },

    { "RDF/XML Parser",
      NS_RDFXMLPARSER_CID,
      NS_RDF_CONTRACTID "/xml-parser;1",
      nsRDFXMLParser::Create
    },

    { "RDF/XML Serializer",
      NS_RDFXMLSERIALIZER_CID,
      NS_RDF_CONTRACTID "/xml-serializer;1",
      nsRDFXMLSerializer::Create
    },

    { "RDF/NTriples Serializer",
      NS_RDFNTRIPLES_SERIALIZER_CID,
      NS_RDF_SERIALIZER "ntriples",
      CreateNewTriplesSerializer
    },

    
    { "Local Store", NS_LOCALSTORE_CID,
      NS_LOCALSTORE_CONTRACTID, NS_NewLocalStore },
};

static nsresult
StartupRDFModule(nsIModule* unused)
{
    if (RDFServiceImpl::gRDFService) {
        NS_ERROR("Leaked the RDF service from a previous startup.");
        RDFServiceImpl::gRDFService = nsnull;
    }

    return NS_OK;
}

static void
ShutdownRDFModule(nsIModule* unused)
{
    if (RDFServiceImpl::gRDFService) {
        
        NS_WARNING("Leaking the RDF Service.");
    }
}

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(nsRDFModule, components,
                                   StartupRDFModule, ShutdownRDFModule)
