




#ifndef nsXULTemplateResultSetRDF_h__
#define nsXULTemplateResultSetRDF_h__

#include "nsISimpleEnumerator.h"
#include "nsRuleNetwork.h"
#include "nsRDFQuery.h"
#include "nsXULTemplateResultRDF.h"
#include "mozilla/Attributes.h"

class nsXULTemplateQueryProcessorRDF;
class nsXULTemplateResultRDF;




class nsXULTemplateResultSetRDF MOZ_FINAL : public nsISimpleEnumerator
{
private:
    nsXULTemplateQueryProcessorRDF* mProcessor;

    nsRDFQuery* mQuery;

    const InstantiationSet* mInstantiations;

    nsCOMPtr<nsIRDFResource> mResource;

    InstantiationSet::List *mCurrent;

    bool mCheckedNext;

    ~nsXULTemplateResultSetRDF()
    {
        delete mInstantiations;
    }

public:

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSISIMPLEENUMERATOR

    nsXULTemplateResultSetRDF(nsXULTemplateQueryProcessorRDF *aProcessor,
                              nsRDFQuery* aQuery,
                              const InstantiationSet* aInstantiations)
        : mProcessor(aProcessor),
          mQuery(aQuery),
          mInstantiations(aInstantiations),
          mCurrent(nullptr),
          mCheckedNext(false)
    { }
};

#endif 
