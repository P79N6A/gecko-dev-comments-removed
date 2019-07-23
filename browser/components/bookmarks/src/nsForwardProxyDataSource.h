




































#ifndef FORWARDPROXYDATASOURCE___H___
#define FORWARDPROXYDATASOURCE___H___

#include "nsCOMArray.h"
#include "nsIRDFService.h"
#include "nsIRDFInferDataSource.h"
#include "nsIRDFDataSource.h"
#include "nsCycleCollectionParticipant.h"

class nsForwardProxyDataSource : public nsIRDFInferDataSource,
                                 public nsIRDFObserver
{
public:
    nsForwardProxyDataSource();

    nsresult Init(void);

    
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsForwardProxyDataSource,
                                             nsIRDFInferDataSource)

    
    NS_DECL_NSIRDFDATASOURCE

    
    NS_DECL_NSIRDFOBSERVER

    
    NS_DECL_NSIRDFINFERDATASOURCE

protected:
    nsCOMPtr<nsIRDFDataSource> mDS;
    nsCOMPtr<nsIRDFResource> mProxyProperty;

    nsCOMArray<nsIRDFObserver> mObservers;

    PRInt32 mUpdateBatchNest;

    nsresult GetProxyResource (nsIRDFResource *aResource, nsIRDFResource **aResult);
    nsresult GetRealSource (nsIRDFResource *aResource, nsIRDFResource **aResult);
    nsresult GetProxyResourcesArray (nsISupportsArray* aSources, nsISupportsArray** aSourcesAndProxies);

    virtual ~nsForwardProxyDataSource();
};

#endif 
