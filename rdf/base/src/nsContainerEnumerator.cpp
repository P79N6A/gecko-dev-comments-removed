






















































#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsIRDFContainerUtils.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFService.h"
#include "nsIServiceManager.h"
#include "nsRDFCID.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "prlog.h"
#include "rdf.h"
#include "rdfutil.h"



static NS_DEFINE_CID(kRDFServiceCID,        NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kRDFContainerUtilsCID, NS_RDFCONTAINERUTILS_CID);



class ContainerEnumeratorImpl : public nsISimpleEnumerator {
private:
    
    static nsrefcnt              gRefCnt;
    static nsIRDFResource*       kRDF_nextVal;
    static nsIRDFContainerUtils* gRDFC;

    nsCOMPtr<nsIRDFDataSource>      mDataSource;
    nsCOMPtr<nsIRDFResource>        mContainer;
    nsCOMPtr<nsIRDFResource>        mOrdinalProperty;

    nsCOMPtr<nsISimpleEnumerator>   mCurrent;
    nsCOMPtr<nsIRDFNode>            mResult;
    PRInt32 mNextIndex;

public:
    ContainerEnumeratorImpl(nsIRDFDataSource* ds, nsIRDFResource* container);
    virtual ~ContainerEnumeratorImpl();

    nsresult Init();

    NS_DECL_ISUPPORTS
    NS_DECL_NSISIMPLEENUMERATOR
};

nsrefcnt              ContainerEnumeratorImpl::gRefCnt;
nsIRDFResource*       ContainerEnumeratorImpl::kRDF_nextVal;
nsIRDFContainerUtils* ContainerEnumeratorImpl::gRDFC;


ContainerEnumeratorImpl::ContainerEnumeratorImpl(nsIRDFDataSource* aDataSource,
                                                 nsIRDFResource* aContainer)
    : mDataSource(aDataSource),
      mContainer(aContainer),
      mNextIndex(1)
{
}

nsresult
ContainerEnumeratorImpl::Init()
{
    if (gRefCnt++ == 0) {
        nsresult rv;
        nsCOMPtr<nsIRDFService> rdf = do_GetService(kRDFServiceCID);
        NS_ASSERTION(rdf != nsnull, "unable to acquire resource manager");
        if (! rdf)
            return NS_ERROR_FAILURE;

        rv = rdf->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "nextVal"), &kRDF_nextVal);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get resource");
        if (NS_FAILED(rv)) return rv;

        rv = CallGetService(kRDFContainerUtilsCID, &gRDFC);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}


ContainerEnumeratorImpl::~ContainerEnumeratorImpl()
{
    if (--gRefCnt == 0) {
        NS_IF_RELEASE(kRDF_nextVal);
        NS_IF_RELEASE(gRDFC);
    }
}

NS_IMPL_ISUPPORTS1(ContainerEnumeratorImpl, nsISimpleEnumerator)


NS_IMETHODIMP
ContainerEnumeratorImpl::HasMoreElements(PRBool* aResult)
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

    
    if (mResult) {
        *aResult = PR_TRUE;
        return NS_OK;
    }

    

    
    
    
    
    
    
    
    
    PRInt32 max = 0;

    nsCOMPtr<nsISimpleEnumerator> targets;
    rv = mDataSource->GetTargets(mContainer, kRDF_nextVal, PR_TRUE, getter_AddRefs(targets));
    if (NS_FAILED(rv)) return rv;

    while (1) {
        PRBool hasmore;
        targets->HasMoreElements(&hasmore);
        if (! hasmore)
            break;

        nsCOMPtr<nsISupports> isupports;
        targets->GetNext(getter_AddRefs(isupports));

        nsCOMPtr<nsIRDFLiteral> nextValLiteral = do_QueryInterface(isupports);
        if (! nextValLiteral)
             continue;

         const PRUnichar *nextValStr;
         nextValLiteral->GetValueConst(&nextValStr);
		 
         PRInt32 err;
         PRInt32 nextVal = nsAutoString(nextValStr).ToInteger(&err);

         if (nextVal > max)
             max = nextVal;
    }

    
    while (mCurrent || mNextIndex < max) {

        
        if (! mCurrent) {
            rv = gRDFC->IndexToOrdinalResource(mNextIndex, getter_AddRefs(mOrdinalProperty));
            if (NS_FAILED(rv)) return rv;

            rv = mDataSource->GetTargets(mContainer, mOrdinalProperty, PR_TRUE, getter_AddRefs(mCurrent));
            if (NS_FAILED(rv)) return rv;

            ++mNextIndex;
        }

        if (mCurrent) {
            PRBool hasMore;
            rv = mCurrent->HasMoreElements(&hasMore);
            if (NS_FAILED(rv)) return rv;

            
            
            if (! hasMore) {
                mCurrent = nsnull;
                continue;
            }

            
            nsCOMPtr<nsISupports> result;
            rv = mCurrent->GetNext(getter_AddRefs(result));
            if (NS_FAILED(rv)) return rv;

            mResult = do_QueryInterface(result, &rv);
            if (NS_FAILED(rv)) return rv;

            *aResult = PR_TRUE;
            return NS_OK;
        }
    }

    
    *aResult = PR_FALSE;
    return NS_OK;
}


NS_IMETHODIMP
ContainerEnumeratorImpl::GetNext(nsISupports** aResult)
{
    nsresult rv;

    PRBool hasMore;
    rv = HasMoreElements(&hasMore);
    if (NS_FAILED(rv)) return rv;

    if (! hasMore)
        return NS_ERROR_UNEXPECTED;

    NS_ADDREF(*aResult = mResult);
    mResult = nsnull;

    return NS_OK;
}




nsresult
NS_NewContainerEnumerator(nsIRDFDataSource* aDataSource,
                          nsIRDFResource* aContainer,
                          nsISimpleEnumerator** aResult)
{
    NS_PRECONDITION(aDataSource != nsnull, "null ptr");
    if (! aDataSource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aContainer != nsnull, "null ptr");
    if (! aContainer)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    ContainerEnumeratorImpl* result = new ContainerEnumeratorImpl(aDataSource, aContainer);
    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(result);

    nsresult rv = result->Init();
    if (NS_FAILED(rv))
        NS_RELEASE(result);

    *aResult = result;
    return rv;
}
