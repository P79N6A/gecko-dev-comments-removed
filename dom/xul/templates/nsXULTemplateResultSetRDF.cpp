




#include "nsXULTemplateResultSetRDF.h"
#include "nsXULTemplateQueryProcessorRDF.h"

NS_IMPL_ISUPPORTS(nsXULTemplateResultSetRDF, nsISimpleEnumerator)

NS_IMETHODIMP
nsXULTemplateResultSetRDF::HasMoreElements(bool *aResult)
{
    *aResult = true;

    nsCOMPtr<nsIRDFNode> node;

    if (! mInstantiations || ! mQuery) {
        *aResult = false;
        return NS_OK;
    }

    if (mCheckedNext) {
        if (!mCurrent || mCurrent == &(mInstantiations->mHead))
            *aResult = false;
        return NS_OK;
    }

    mCheckedNext = true;
                
    do {
        if (mCurrent) {
            mCurrent = mCurrent->mNext;
            if (mCurrent == &(mInstantiations->mHead)) {
                *aResult = false;
                return NS_OK;
            }
        }
        else {
            *aResult = ! mInstantiations->Empty();
            if (*aResult)
                mCurrent = mInstantiations->mHead.mNext;
        }

        
        
        if (mCurrent) {
            mCurrent->mInstantiation.mAssignments.
                GetAssignmentFor(mQuery->mMemberVariable, getter_AddRefs(node));
        }

        
        mResource = do_QueryInterface(node);
    } while (! mResource);

    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateResultSetRDF::GetNext(nsISupports **aResult)
{
    if (!aResult)
        return NS_ERROR_NULL_POINTER;

    if (!mCurrent || !mCheckedNext)
        return NS_ERROR_FAILURE;

    nsRefPtr<nsXULTemplateResultRDF> nextresult =
        new nsXULTemplateResultRDF(mQuery, mCurrent->mInstantiation, mResource);
    if (!nextresult)
        return NS_ERROR_OUT_OF_MEMORY;

    
    
    mProcessor->AddMemoryElements(mCurrent->mInstantiation, nextresult);

    mCheckedNext = false;

    nextresult.forget(aResult);

    return NS_OK;
}
