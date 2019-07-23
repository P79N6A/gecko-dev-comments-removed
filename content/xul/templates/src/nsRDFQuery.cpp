



































#include "nscore.h"
#include "nsCOMPtr.h"

#include "nsXULTemplateQueryProcessorRDF.h"
#include "nsRDFQuery.h"

NS_IMPL_CYCLE_COLLECTION_1(nsRDFQuery, mQueryNode)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsRDFQuery)
  NS_INTERFACE_MAP_ENTRY(nsITemplateRDFQuery)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsRDFQuery)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsRDFQuery)

void
nsRDFQuery::Finish()
{
    
    
    mProcessor = nsnull;
    mCachedResults = nsnull;
}

nsresult
nsRDFQuery::SetCachedResults(nsXULTemplateQueryProcessorRDF* aProcessor,
                             const InstantiationSet& aInstantiations)
{
    mCachedResults = new nsXULTemplateResultSetRDF(aProcessor, this, &aInstantiations);
    if (! mCachedResults)
        return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}


void
nsRDFQuery::UseCachedResults(nsISimpleEnumerator** aResults)
{
    *aResults = mCachedResults;
    NS_IF_ADDREF(*aResults);

    mCachedResults = nsnull;
}
