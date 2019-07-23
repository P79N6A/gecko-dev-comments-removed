






























































#include "xpcom-config.h"
#include NEW_H
#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsIEnumerator.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFObserver.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsFixedSizeAllocator.h"
#include "nsVoidArray.h"
#include "nsCOMArray.h"
#include "nsArrayEnumerator.h"
#include "nsXPIDLString.h"
#include "rdf.h"
#include "nsCycleCollectionParticipant.h"

#include "nsEnumeratorUtils.h"

#ifdef NS_DEBUG
#include "prlog.h"
#include "prprf.h"
#include <stdio.h>
PRLogModuleInfo* nsRDFLog = nsnull;
#endif

static NS_DEFINE_IID(kISupportsIID,           NS_ISUPPORTS_IID);






class CompositeEnumeratorImpl;
class CompositeArcsInOutEnumeratorImpl;
class CompositeAssertionEnumeratorImpl;

class CompositeDataSourceImpl : public nsIRDFCompositeDataSource,
                                public nsIRDFObserver
{
public:
    CompositeDataSourceImpl(void);
    CompositeDataSourceImpl(char** dataSources);

    
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(CompositeDataSourceImpl,
                                             nsIRDFCompositeDataSource)

    
    NS_DECL_NSIRDFDATASOURCE

    
    NS_DECL_NSIRDFCOMPOSITEDATASOURCE

    
    NS_DECL_NSIRDFOBSERVER

    PRBool HasAssertionN(int n, nsIRDFResource* source,
                            nsIRDFResource* property,
                            nsIRDFNode* target,
                            PRBool tv);

protected:
    nsCOMArray<nsIRDFObserver> mObservers;
    nsCOMArray<nsIRDFDataSource> mDataSources;

	PRBool      mAllowNegativeAssertions;
	PRBool      mCoalesceDuplicateArcs;
    PRInt32     mUpdateBatchNest;

    nsFixedSizeAllocator mAllocator;

	virtual ~CompositeDataSourceImpl() {}

    friend class CompositeEnumeratorImpl;
    friend class CompositeArcsInOutEnumeratorImpl;
    friend class CompositeAssertionEnumeratorImpl;
};






class CompositeEnumeratorImpl : public nsISimpleEnumerator
{
    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSISIMPLEENUMERATOR

    
    virtual nsresult
    GetEnumerator(nsIRDFDataSource* aDataSource, nsISimpleEnumerator** aResult) = 0;

    virtual nsresult
    HasNegation(nsIRDFDataSource* aDataSource, nsIRDFNode* aNode, PRBool* aResult) = 0;

    virtual void Destroy() = 0;

protected:
    CompositeEnumeratorImpl(CompositeDataSourceImpl* aCompositeDataSource,
                            PRBool aAllowNegativeAssertions,
                            PRBool aCoalesceDuplicateArcs);

    virtual ~CompositeEnumeratorImpl();
    
    CompositeDataSourceImpl* mCompositeDataSource;

    nsISimpleEnumerator* mCurrent;
    nsIRDFNode*  mResult;
    PRInt32      mNext;
    nsAutoVoidArray  mAlreadyReturned;
    PRPackedBool mAllowNegativeAssertions;
    PRPackedBool mCoalesceDuplicateArcs;
};


CompositeEnumeratorImpl::CompositeEnumeratorImpl(CompositeDataSourceImpl* aCompositeDataSource,
                                                 PRBool aAllowNegativeAssertions,
                                                 PRBool aCoalesceDuplicateArcs)
    : mCompositeDataSource(aCompositeDataSource),
      mCurrent(nsnull),
      mResult(nsnull),
	  mNext(0),
      mAllowNegativeAssertions(aAllowNegativeAssertions),
      mCoalesceDuplicateArcs(aCoalesceDuplicateArcs)
{
	NS_ADDREF(mCompositeDataSource);
}


CompositeEnumeratorImpl::~CompositeEnumeratorImpl(void)
{
	if (mCoalesceDuplicateArcs == PR_TRUE)
	{
		for (PRInt32 i = mAlreadyReturned.Count() - 1; i >= 0; --i)
		{
			nsIRDFNode *node = (nsIRDFNode *) mAlreadyReturned[i];
			NS_RELEASE(node);
		}
	}

	NS_IF_RELEASE(mCurrent);
	NS_IF_RELEASE(mResult);
	NS_RELEASE(mCompositeDataSource);
}

NS_IMPL_ADDREF(CompositeEnumeratorImpl)
NS_IMPL_RELEASE_WITH_DESTROY(CompositeEnumeratorImpl, Destroy())
NS_IMPL_QUERY_INTERFACE1(CompositeEnumeratorImpl, nsISimpleEnumerator)

NS_IMETHODIMP
CompositeEnumeratorImpl::HasMoreElements(PRBool* aResult)
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

    
    
    if (mResult) {
        *aResult = PR_TRUE;
        return NS_OK;
    }

    
    
    for ( ; mNext < mCompositeDataSource->mDataSources.Count(); ++mNext) {
        if (! mCurrent) {
            
            
            nsIRDFDataSource* datasource =
                mCompositeDataSource->mDataSources[mNext];

            rv = GetEnumerator(datasource, &mCurrent);
            if (NS_FAILED(rv)) return rv;
            if (rv == NS_RDF_NO_VALUE)
                continue;

            NS_ASSERTION(mCurrent != nsnull, "you're always supposed to return an enumerator from GetEnumerator, punk.");
            if (! mCurrent)
                continue;
        }

        do {
            PRInt32 i;

            PRBool hasMore;
            rv = mCurrent->HasMoreElements(&hasMore);
            if (NS_FAILED(rv)) return rv;

            
            if (! hasMore) {
                NS_RELEASE(mCurrent);
                break;
            }

            
            
            

            
            nsCOMPtr<nsISupports> result;
            rv = mCurrent->GetNext(getter_AddRefs(result));
            if (NS_FAILED(rv)) return rv;

            rv = result->QueryInterface(NS_GET_IID(nsIRDFNode), (void**) &mResult);
            if (NS_FAILED(rv)) return rv;

            if (mAllowNegativeAssertions == PR_TRUE)
            {
                
                PRBool hasNegation = PR_FALSE;
                for (i = mNext - 1; i >= 0; --i)
                {
                    nsIRDFDataSource* datasource =
                        mCompositeDataSource->mDataSources[i];

                    rv = HasNegation(datasource, mResult, &hasNegation);
                    if (NS_FAILED(rv)) return rv;

                    if (hasNegation)
                        break;
                }

                
                if (hasNegation)
                {
                    NS_RELEASE(mResult);
                    continue;
                }
            }

            if (mCoalesceDuplicateArcs == PR_TRUE)
            {
                
                
                PRBool alreadyReturned = PR_FALSE;
                for (i = mAlreadyReturned.Count() - 1; i >= 0; --i)
                {
                    if (mAlreadyReturned[i] == mResult)
                    {
                        alreadyReturned = PR_TRUE;
                        break;
                    }
                }
                if (alreadyReturned == PR_TRUE)
                {
                    NS_RELEASE(mResult);
                    continue;
                }
            }

            
            
            *aResult = PR_TRUE;

            

            
            
            

            if (mCoalesceDuplicateArcs == PR_TRUE)
            {
                mAlreadyReturned.AppendElement(mResult);
                NS_ADDREF(mResult);
            }

            return NS_OK;
        } while (1);
    }

    
    *aResult = PR_FALSE;
    return NS_OK;
}


NS_IMETHODIMP
CompositeEnumeratorImpl::GetNext(nsISupports** aResult)
{
    nsresult rv;

    PRBool hasMore;
    rv = HasMoreElements(&hasMore);
    if (NS_FAILED(rv)) return rv;

    if (! hasMore)
        return NS_ERROR_UNEXPECTED;

    
    *aResult = mResult;
    mResult = nsnull;

    return NS_OK;
}







class CompositeArcsInOutEnumeratorImpl : public CompositeEnumeratorImpl
{
public:
    enum Type { eArcsIn, eArcsOut };

    static CompositeArcsInOutEnumeratorImpl*
    Create(nsFixedSizeAllocator& aAllocator,
           CompositeDataSourceImpl* aCompositeDataSource,
           nsIRDFNode* aNode,
           Type aType,
           PRBool aAllowNegativeAssertions,
           PRBool aCoalesceDuplicateArcs) {
        void* place = aAllocator.Alloc(sizeof(CompositeArcsInOutEnumeratorImpl));
        return place
            ? ::new (place) CompositeArcsInOutEnumeratorImpl(aCompositeDataSource,
                                                             aNode, aType,
                                                             aAllowNegativeAssertions,
                                                             aCoalesceDuplicateArcs)
            : nsnull; }

    virtual ~CompositeArcsInOutEnumeratorImpl();

    virtual nsresult
    GetEnumerator(nsIRDFDataSource* aDataSource, nsISimpleEnumerator** aResult);

    virtual nsresult
    HasNegation(nsIRDFDataSource* aDataSource, nsIRDFNode* aNode, PRBool* aResult);

    virtual void Destroy();

protected:
    CompositeArcsInOutEnumeratorImpl(CompositeDataSourceImpl* aCompositeDataSource,
                                     nsIRDFNode* aNode,
                                     Type aType,
                                     PRBool aAllowNegativeAssertions,
                                     PRBool aCoalesceDuplicateArcs);

private:
    nsIRDFNode* mNode;
    Type        mType;
    PRBool	    mAllowNegativeAssertions;
    PRBool      mCoalesceDuplicateArcs;

    
    
    static void* operator new(size_t) CPP_THROW_NEW { return 0; }
    static void operator delete(void*, size_t) {}
};


CompositeArcsInOutEnumeratorImpl::CompositeArcsInOutEnumeratorImpl(
                CompositeDataSourceImpl* aCompositeDataSource,
                nsIRDFNode* aNode,
                Type aType,
                PRBool aAllowNegativeAssertions,
                PRBool aCoalesceDuplicateArcs)
    : CompositeEnumeratorImpl(aCompositeDataSource, aAllowNegativeAssertions, aCoalesceDuplicateArcs),
      mNode(aNode),
      mType(aType),
      mAllowNegativeAssertions(aAllowNegativeAssertions),
      mCoalesceDuplicateArcs(aCoalesceDuplicateArcs)
{
    NS_ADDREF(mNode);
}

CompositeArcsInOutEnumeratorImpl::~CompositeArcsInOutEnumeratorImpl()
{
    NS_RELEASE(mNode);
}


nsresult
CompositeArcsInOutEnumeratorImpl::GetEnumerator(
                 nsIRDFDataSource* aDataSource,
                 nsISimpleEnumerator** aResult)
{
    if (mType == eArcsIn) {
        return aDataSource->ArcLabelsIn(mNode, aResult);
    }
    else {
        nsCOMPtr<nsIRDFResource> resource( do_QueryInterface(mNode) );
        return aDataSource->ArcLabelsOut(resource, aResult);
    }
}

nsresult
CompositeArcsInOutEnumeratorImpl::HasNegation(
                 nsIRDFDataSource* aDataSource,
                 nsIRDFNode* aNode,
                 PRBool* aResult)
{
    *aResult = PR_FALSE;
    return NS_OK;
}

void
CompositeArcsInOutEnumeratorImpl::Destroy()
{
    
    
    nsCOMPtr<nsIRDFCompositeDataSource> kungFuDeathGrip = mCompositeDataSource;

    nsFixedSizeAllocator& pool = mCompositeDataSource->mAllocator;
    this->~CompositeArcsInOutEnumeratorImpl();
    pool.Free(this, sizeof(*this));
}







class CompositeAssertionEnumeratorImpl : public CompositeEnumeratorImpl
{
public:
    static CompositeAssertionEnumeratorImpl*
    Create(nsFixedSizeAllocator& aAllocator,
           CompositeDataSourceImpl* aCompositeDataSource,
           nsIRDFResource* aSource,
           nsIRDFResource* aProperty,
           nsIRDFNode* aTarget,
           PRBool aTruthValue,
           PRBool aAllowNegativeAssertions,
           PRBool aCoalesceDuplicateArcs) {
        void* place = aAllocator.Alloc(sizeof(CompositeAssertionEnumeratorImpl));
        return place
            ? ::new (place) CompositeAssertionEnumeratorImpl(aCompositeDataSource,
                                                             aSource, aProperty, aTarget,
                                                             aTruthValue,
                                                             aAllowNegativeAssertions,
                                                             aCoalesceDuplicateArcs)
            : nsnull; }

    virtual nsresult
    GetEnumerator(nsIRDFDataSource* aDataSource, nsISimpleEnumerator** aResult);

    virtual nsresult
    HasNegation(nsIRDFDataSource* aDataSource, nsIRDFNode* aNode, PRBool* aResult);

    virtual void Destroy();

protected:
    CompositeAssertionEnumeratorImpl(CompositeDataSourceImpl* aCompositeDataSource,
                                     nsIRDFResource* aSource,
                                     nsIRDFResource* aProperty,
                                     nsIRDFNode* aTarget,
                                     PRBool aTruthValue,
                                     PRBool aAllowNegativeAssertions,
                                     PRBool aCoalesceDuplicateArcs);

    virtual ~CompositeAssertionEnumeratorImpl();

private:
    nsIRDFResource* mSource;
    nsIRDFResource* mProperty;
    nsIRDFNode*     mTarget;
    PRBool          mTruthValue;
    PRBool          mAllowNegativeAssertions;
    PRBool          mCoalesceDuplicateArcs;

    
    
    static void* operator new(size_t) CPP_THROW_NEW { return 0; }
    static void operator delete(void*, size_t) {}
};


CompositeAssertionEnumeratorImpl::CompositeAssertionEnumeratorImpl(
                  CompositeDataSourceImpl* aCompositeDataSource,
                  nsIRDFResource* aSource,
                  nsIRDFResource* aProperty,
                  nsIRDFNode* aTarget,
                  PRBool aTruthValue,
                  PRBool aAllowNegativeAssertions,
                  PRBool aCoalesceDuplicateArcs)
    : CompositeEnumeratorImpl(aCompositeDataSource, aAllowNegativeAssertions, aCoalesceDuplicateArcs),
      mSource(aSource),
      mProperty(aProperty),
      mTarget(aTarget),
      mTruthValue(aTruthValue),
      mAllowNegativeAssertions(aAllowNegativeAssertions),
      mCoalesceDuplicateArcs(aCoalesceDuplicateArcs)
{
    NS_IF_ADDREF(mSource);
    NS_ADDREF(mProperty); 
    NS_IF_ADDREF(mTarget);
}

CompositeAssertionEnumeratorImpl::~CompositeAssertionEnumeratorImpl()
{
    NS_IF_RELEASE(mSource);
    NS_RELEASE(mProperty);
    NS_IF_RELEASE(mTarget);
}


nsresult
CompositeAssertionEnumeratorImpl::GetEnumerator(
                 nsIRDFDataSource* aDataSource,
                 nsISimpleEnumerator** aResult)
{
    if (mSource) {
        return aDataSource->GetTargets(mSource, mProperty, mTruthValue, aResult);
    }
    else {
        return aDataSource->GetSources(mProperty, mTarget, mTruthValue, aResult);
    }
}

nsresult
CompositeAssertionEnumeratorImpl::HasNegation(
                 nsIRDFDataSource* aDataSource,
                 nsIRDFNode* aNode,
                 PRBool* aResult)
{
    if (mSource) {
        return aDataSource->HasAssertion(mSource, mProperty, aNode, !mTruthValue, aResult);
    }
    else {
        nsCOMPtr<nsIRDFResource> source( do_QueryInterface(aNode) );
        return aDataSource->HasAssertion(source, mProperty, mTarget, !mTruthValue, aResult);
    }
}

void
CompositeAssertionEnumeratorImpl::Destroy()
{
    
    
    nsCOMPtr<nsIRDFCompositeDataSource> kungFuDeathGrip = mCompositeDataSource;

    nsFixedSizeAllocator& pool = mCompositeDataSource->mAllocator;
    this->~CompositeAssertionEnumeratorImpl();
    pool.Free(this, sizeof(*this));
}



nsresult
NS_NewRDFCompositeDataSource(nsIRDFCompositeDataSource** result)
{
    CompositeDataSourceImpl* db = new CompositeDataSourceImpl();
    if (! db)
        return NS_ERROR_OUT_OF_MEMORY;

    *result = db;
    NS_ADDREF(*result);
    return NS_OK;
}


CompositeDataSourceImpl::CompositeDataSourceImpl(void)
	: mAllowNegativeAssertions(PR_TRUE),
	  mCoalesceDuplicateArcs(PR_TRUE),
      mUpdateBatchNest(0)
{
    static const size_t kBucketSizes[] = {
        sizeof(CompositeAssertionEnumeratorImpl),
        sizeof(CompositeArcsInOutEnumeratorImpl) };

    static const PRInt32 kNumBuckets = sizeof(kBucketSizes) / sizeof(size_t);

    
    static const PRInt32 kInitialSize = 256;

    mAllocator.Init("nsCompositeDataSource", kBucketSizes, kNumBuckets, kInitialSize);

#ifdef PR_LOGGING
    if (nsRDFLog == nsnull) 
        nsRDFLog = PR_NewLogModule("RDF");
#endif
}






NS_IMPL_CYCLE_COLLECTION_CLASS(CompositeDataSourceImpl)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(CompositeDataSourceImpl)
    PRUint32 i, count = tmp->mDataSources.Count();
    for (i = count; i > 0; --i) {
        tmp->mDataSources[i - 1]->RemoveObserver(tmp);
        tmp->mDataSources.RemoveObjectAt(i - 1);
    }
    NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mObservers);
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(CompositeDataSourceImpl)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mObservers)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mDataSources)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END


NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(CompositeDataSourceImpl,
                                          nsIRDFCompositeDataSource)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(CompositeDataSourceImpl,
                                           nsIRDFCompositeDataSource)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(CompositeDataSourceImpl)
    NS_INTERFACE_MAP_ENTRY(nsIRDFCompositeDataSource)
    NS_INTERFACE_MAP_ENTRY(nsIRDFDataSource)
    NS_INTERFACE_MAP_ENTRY(nsIRDFObserver)
    NS_INTERFACE_MAP_ENTRY(nsIRDFCompositeDataSource)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIRDFCompositeDataSource)
NS_INTERFACE_MAP_END







NS_IMETHODIMP
CompositeDataSourceImpl::GetURI(char* *uri)
{
    *uri = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
CompositeDataSourceImpl::GetSource(nsIRDFResource* property,
                                   nsIRDFNode* target,
                                   PRBool tv,
                                   nsIRDFResource** source)
{
	if ((mAllowNegativeAssertions == PR_FALSE) && (tv == PR_FALSE))
		return(NS_RDF_NO_VALUE);

    PRInt32 count = mDataSources.Count();
    for (PRInt32 i = 0; i < count; ++i) {
        nsresult rv;
        rv = mDataSources[i]->GetSource(property, target, tv, source);
        if (NS_FAILED(rv)) return rv;

        if (rv == NS_RDF_NO_VALUE)
            continue;

        if (mAllowNegativeAssertions == PR_FALSE)	return(NS_OK);

        
        
        if (!HasAssertionN(count-1, *source, property, target, !tv)) 
            return NS_OK;

        NS_RELEASE(*source);
        return NS_RDF_NO_VALUE;
    }
    return NS_RDF_NO_VALUE;
}

NS_IMETHODIMP
CompositeDataSourceImpl::GetSources(nsIRDFResource* aProperty,
                                    nsIRDFNode* aTarget,
                                    PRBool aTruthValue,
                                    nsISimpleEnumerator** aResult)
{
    NS_PRECONDITION(aProperty != nsnull, "null ptr");
    if (! aProperty)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aTarget != nsnull, "null ptr");
    if (! aTarget)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    if ((mAllowNegativeAssertions == PR_FALSE) && (aTruthValue == PR_FALSE))
        return(NS_RDF_NO_VALUE);

    *aResult = CompositeAssertionEnumeratorImpl::Create(mAllocator,
                                                        this, nsnull, aProperty,
                                                        aTarget, aTruthValue,
                                                        mAllowNegativeAssertions,
                                                        mCoalesceDuplicateArcs);

    if (! *aResult)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aResult);
    return NS_OK;
}

NS_IMETHODIMP
CompositeDataSourceImpl::GetTarget(nsIRDFResource* aSource,
                                   nsIRDFResource* aProperty,
                                   PRBool aTruthValue,
                                   nsIRDFNode** aResult)
{
    NS_PRECONDITION(aSource != nsnull, "null ptr");
    if (! aSource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aProperty != nsnull, "null ptr");
    if (! aProperty)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    if ((mAllowNegativeAssertions == PR_FALSE) && (aTruthValue == PR_FALSE))
        return(NS_RDF_NO_VALUE);

    PRInt32 count = mDataSources.Count();
    for (PRInt32 i = 0; i < count; ++i) {
        nsresult rv;
        rv = mDataSources[i]->GetTarget(aSource, aProperty, aTruthValue,
                                        aResult);
        if (NS_FAILED(rv))
            return rv;

        if (rv == NS_OK) {
            
            

            if (mAllowNegativeAssertions == PR_TRUE) {
                if (HasAssertionN(count-1, aSource, aProperty, *aResult, !aTruthValue)) {
                    
                    NS_RELEASE(*aResult);
                    return NS_RDF_NO_VALUE;
                }
            }
            return NS_OK;
        }
    }

    
    return NS_RDF_NO_VALUE;
}

PRBool
CompositeDataSourceImpl::HasAssertionN(int n,
                                       nsIRDFResource* aSource,
                                       nsIRDFResource* aProperty,
                                       nsIRDFNode* aTarget,
                                       PRBool aTruthValue)
{
    nsresult rv;
    for (PRInt32 m = 0; m < n; ++m) {
        PRBool result;
        rv = mDataSources[m]->HasAssertion(aSource, aProperty, aTarget,
                                           aTruthValue, &result);
        if (NS_FAILED(rv))
            return PR_FALSE;

        
        if (result)
            return PR_TRUE;
    }
    return PR_FALSE;
}
    


NS_IMETHODIMP
CompositeDataSourceImpl::GetTargets(nsIRDFResource* aSource,
                                    nsIRDFResource* aProperty,
                                    PRBool aTruthValue,
                                    nsISimpleEnumerator** aResult)
{
    NS_PRECONDITION(aSource != nsnull, "null ptr");
    if (! aSource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aProperty != nsnull, "null ptr");
    if (! aProperty)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    if ((mAllowNegativeAssertions == PR_FALSE) && (aTruthValue == PR_FALSE))
        return(NS_RDF_NO_VALUE);

    *aResult =
        CompositeAssertionEnumeratorImpl::Create(mAllocator, this,
                                                 aSource, aProperty, nsnull,
                                                 aTruthValue,
                                                 mAllowNegativeAssertions,
                                                 mCoalesceDuplicateArcs);

    if (! *aResult)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aResult);
    return NS_OK;
}

NS_IMETHODIMP
CompositeDataSourceImpl::Assert(nsIRDFResource* aSource, 
                                nsIRDFResource* aProperty, 
                                nsIRDFNode* aTarget,
                                PRBool aTruthValue)
{
    NS_PRECONDITION(aSource != nsnull, "null ptr");
    if (! aSource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aProperty != nsnull, "null ptr");
    if (! aProperty)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aTarget != nsnull, "null ptr");
    if (! aTarget)
        return NS_ERROR_NULL_POINTER;

    if ((mAllowNegativeAssertions == PR_FALSE) && (aTruthValue == PR_FALSE))
        return(NS_RDF_ASSERTION_REJECTED);

    nsresult rv;

    

    
    
    
    for (PRInt32 i = mDataSources.Count() - 1; i >= 0; --i) {
        rv = mDataSources[i]->Assert(aSource, aProperty, aTarget, aTruthValue);
        if (NS_RDF_ASSERTION_ACCEPTED == rv)
            return rv;

        if (NS_FAILED(rv))
            return rv;
    }

    
    return NS_RDF_ASSERTION_REJECTED;
}

NS_IMETHODIMP
CompositeDataSourceImpl::Unassert(nsIRDFResource* aSource,
                                  nsIRDFResource* aProperty,
                                  nsIRDFNode* aTarget)
{
    NS_PRECONDITION(aSource != nsnull, "null ptr");
    if (! aSource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aProperty != nsnull, "null ptr");
    if (! aProperty)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aTarget != nsnull, "null ptr");
    if (! aTarget)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

    
    
    
    PRBool unasserted = PR_TRUE;
    PRInt32 i;
    PRInt32 count = mDataSources.Count();
    for (i = 0; i < count; ++i) {
        nsIRDFDataSource* ds = mDataSources[i];

        PRBool hasAssertion;
        rv = ds->HasAssertion(aSource, aProperty, aTarget, PR_TRUE, &hasAssertion);
        if (NS_FAILED(rv)) return rv;

        if (hasAssertion) {
            rv = ds->Unassert(aSource, aProperty, aTarget);
            if (NS_FAILED(rv)) return rv;

            if (rv != NS_RDF_ASSERTION_ACCEPTED) {
                unasserted = PR_FALSE;
                break;
            }
        }
    }

    
    
    if (unasserted)
        return NS_RDF_ASSERTION_ACCEPTED;

    
    
    
    
    for (i = 0; i < count; ++i) {
        rv = mDataSources[i]->Assert(aSource, aProperty, aTarget, PR_FALSE);
        if (NS_FAILED(rv)) return rv;

        
        if (rv == NS_RDF_ASSERTION_ACCEPTED)
            return rv;
    }

    
    return NS_RDF_ASSERTION_REJECTED;
}

NS_IMETHODIMP
CompositeDataSourceImpl::Change(nsIRDFResource* aSource,
                                nsIRDFResource* aProperty,
                                nsIRDFNode* aOldTarget,
                                nsIRDFNode* aNewTarget)
{
    NS_PRECONDITION(aSource != nsnull, "null ptr");
    if (! aSource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aProperty != nsnull, "null ptr");
    if (! aProperty)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aOldTarget != nsnull, "null ptr");
    if (! aOldTarget)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aNewTarget != nsnull, "null ptr");
    if (! aNewTarget)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

    
    
    

    
    
    
    for (PRInt32 i = mDataSources.Count() - 1; i >= 0; --i) {
        rv = mDataSources[i]->Change(aSource, aProperty, aOldTarget, aNewTarget);
        if (NS_RDF_ASSERTION_ACCEPTED == rv)
            return rv;

        if (NS_FAILED(rv))
            return rv;
    }

    
    return NS_RDF_ASSERTION_REJECTED;
}

NS_IMETHODIMP
CompositeDataSourceImpl::Move(nsIRDFResource* aOldSource,
                              nsIRDFResource* aNewSource,
                              nsIRDFResource* aProperty,
                              nsIRDFNode* aTarget)
{
    NS_PRECONDITION(aOldSource != nsnull, "null ptr");
    if (! aOldSource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aNewSource != nsnull, "null ptr");
    if (! aNewSource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aProperty != nsnull, "null ptr");
    if (! aProperty)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aTarget != nsnull, "null ptr");
    if (! aTarget)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

    
    
    

    
    
    
    for (PRInt32 i = mDataSources.Count() - 1; i >= 0; --i) {
        rv = mDataSources[i]->Move(aOldSource, aNewSource, aProperty, aTarget);
        if (NS_RDF_ASSERTION_ACCEPTED == rv)
            return rv;

        if (NS_FAILED(rv))
            return rv;
    }

    
    return NS_RDF_ASSERTION_REJECTED;
}


NS_IMETHODIMP
CompositeDataSourceImpl::HasAssertion(nsIRDFResource* aSource,
                                      nsIRDFResource* aProperty,
                                      nsIRDFNode* aTarget,
                                      PRBool aTruthValue,
                                      PRBool* aResult)
{
    NS_PRECONDITION(aSource != nsnull, "null ptr");
    if (! aSource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aProperty != nsnull, "null ptr");
    if (! aProperty)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    if ((mAllowNegativeAssertions == PR_FALSE) && (aTruthValue == PR_FALSE))
    {
        *aResult = PR_FALSE;
        return(NS_OK);
    }

    nsresult rv;

    
    
    PRInt32 count = mDataSources.Count();
    for (PRInt32 i = 0; i < count; ++i) {
        nsIRDFDataSource* datasource = mDataSources[i];
        rv = datasource->HasAssertion(aSource, aProperty, aTarget, aTruthValue, aResult);
        if (NS_FAILED(rv)) return rv;

        if (*aResult)
            return NS_OK;

        if (mAllowNegativeAssertions == PR_TRUE)
        {
            PRBool hasNegation;
            rv = datasource->HasAssertion(aSource, aProperty, aTarget, !aTruthValue, &hasNegation);
            if (NS_FAILED(rv)) return rv;

            if (hasNegation)
            {
                *aResult = PR_FALSE;
                return NS_OK;
            }
        }
    }

    
    *aResult = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
CompositeDataSourceImpl::AddObserver(nsIRDFObserver* aObserver)
{
    NS_PRECONDITION(aObserver != nsnull, "null ptr");
    if (! aObserver)
        return NS_ERROR_NULL_POINTER;

    
    mObservers.AppendObject(aObserver);

    return NS_OK;
}

NS_IMETHODIMP
CompositeDataSourceImpl::RemoveObserver(nsIRDFObserver* aObserver)
{
    NS_PRECONDITION(aObserver != nsnull, "null ptr");
    if (! aObserver)
        return NS_ERROR_NULL_POINTER;

    mObservers.RemoveObject(aObserver);

    return NS_OK;
}

NS_IMETHODIMP 
CompositeDataSourceImpl::HasArcIn(nsIRDFNode *aNode, nsIRDFResource *aArc, PRBool *result)
{
    nsresult rv;
    *result = PR_FALSE;
    PRInt32 count = mDataSources.Count();
    for (PRInt32 i = 0; i < count; ++i) {
        rv = mDataSources[i]->HasArcIn(aNode, aArc, result);
        if (NS_FAILED(rv)) return rv;
        if (*result == PR_TRUE)
            return NS_OK;
    }
    return NS_OK;
}

NS_IMETHODIMP 
CompositeDataSourceImpl::HasArcOut(nsIRDFResource *aSource, nsIRDFResource *aArc, PRBool *result)
{
    nsresult rv;
    *result = PR_FALSE;
    PRInt32 count = mDataSources.Count();
    for (PRInt32 i = 0; i < count; ++i) {
        rv = mDataSources[i]->HasArcOut(aSource, aArc, result);
        if (NS_FAILED(rv)) return rv;
        if (*result == PR_TRUE)
            return NS_OK;
    }
    return NS_OK;
}

NS_IMETHODIMP
CompositeDataSourceImpl::ArcLabelsIn(nsIRDFNode* aTarget, nsISimpleEnumerator** aResult)
{
    NS_PRECONDITION(aTarget != nsnull, "null ptr");
    if (! aTarget)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    nsISimpleEnumerator* result = 
        CompositeArcsInOutEnumeratorImpl::Create(mAllocator, this, aTarget,
                                                 CompositeArcsInOutEnumeratorImpl::eArcsIn,
                                                 mAllowNegativeAssertions,
                                                 mCoalesceDuplicateArcs);

    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(result);
    *aResult = result;
    return NS_OK;
}

NS_IMETHODIMP
CompositeDataSourceImpl::ArcLabelsOut(nsIRDFResource* aSource,
                                      nsISimpleEnumerator** aResult)
{
    NS_PRECONDITION(aSource != nsnull, "null ptr");
    if (! aSource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    nsISimpleEnumerator* result =
        CompositeArcsInOutEnumeratorImpl::Create(mAllocator, this, aSource,
                                                 CompositeArcsInOutEnumeratorImpl::eArcsOut,
                                                 mAllowNegativeAssertions,
                                                 mCoalesceDuplicateArcs);

    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(result);
    *aResult = result;
    return NS_OK;
}

NS_IMETHODIMP
CompositeDataSourceImpl::GetAllResources(nsISimpleEnumerator** aResult)
{
    NS_NOTYETIMPLEMENTED("CompositeDataSourceImpl::GetAllResources");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
CompositeDataSourceImpl::GetAllCmds(nsIRDFResource* source,
                                    nsISimpleEnumerator** result)
{
    nsCOMPtr<nsISupportsArray> cmdArray;
    nsresult rv;

    rv = NS_NewISupportsArray(getter_AddRefs(cmdArray));
    if (NS_FAILED(rv)) return(rv);

    for (PRInt32 i = 0; i < mDataSources.Count(); i++)
    {
        nsCOMPtr<nsISimpleEnumerator> dsCmds;

        rv = mDataSources[i]->GetAllCmds(source, getter_AddRefs(dsCmds));
        if (NS_SUCCEEDED(rv))
        {
            PRBool	hasMore = PR_FALSE;
            while(NS_SUCCEEDED(rv = dsCmds->HasMoreElements(&hasMore)) && (hasMore == PR_TRUE))
            {
                nsCOMPtr<nsISupports>	item;
                if (NS_SUCCEEDED(rv = dsCmds->GetNext(getter_AddRefs(item))))
                {
                    
                    
                    cmdArray->AppendElement(item);
                }
            }
            if (NS_FAILED(rv)) return(rv);
        }
    }

    return NS_NewArrayEnumerator(result, cmdArray);
}

NS_IMETHODIMP
CompositeDataSourceImpl::IsCommandEnabled(nsISupportsArray* aSources,
                                          nsIRDFResource*   aCommand,
                                          nsISupportsArray* aArguments,
                                          PRBool* aResult)
{
    nsresult rv;
    for (PRInt32 i = mDataSources.Count() - 1; i >= 0; --i) {
        PRBool enabled = PR_TRUE;
        rv = mDataSources[i]->IsCommandEnabled(aSources, aCommand, aArguments, &enabled);
        if (NS_FAILED(rv) && (rv != NS_ERROR_NOT_IMPLEMENTED))
        {
            return(rv);
        }

        if (! enabled) {
            *aResult = PR_FALSE;
            return(NS_OK);
        }
    }
    *aResult = PR_TRUE;
    return(NS_OK);
}

NS_IMETHODIMP
CompositeDataSourceImpl::DoCommand(nsISupportsArray* aSources,
                                   nsIRDFResource*   aCommand,
                                   nsISupportsArray* aArguments)
{
    for (PRInt32 i = mDataSources.Count() - 1; i >= 0; --i) {
        nsresult rv = mDataSources[i]->DoCommand(aSources, aCommand, aArguments);
        if (NS_FAILED(rv) && (rv != NS_ERROR_NOT_IMPLEMENTED))
        {
            return(rv);   
        }
    }
    return(NS_OK);
}

NS_IMETHODIMP
CompositeDataSourceImpl::BeginUpdateBatch()
{
    for (PRInt32 i = mDataSources.Count() - 1; i >= 0; --i) {
        mDataSources[i]->BeginUpdateBatch();
    }
    return NS_OK;
}

NS_IMETHODIMP
CompositeDataSourceImpl::EndUpdateBatch()
{
    for (PRInt32 i = mDataSources.Count() - 1; i >= 0; --i) {
        mDataSources[i]->EndUpdateBatch();
    }
    return NS_OK;
}








NS_IMETHODIMP
CompositeDataSourceImpl::GetAllowNegativeAssertions(PRBool *aAllowNegativeAssertions)
{
	*aAllowNegativeAssertions = mAllowNegativeAssertions;
	return(NS_OK);
}

NS_IMETHODIMP
CompositeDataSourceImpl::SetAllowNegativeAssertions(PRBool aAllowNegativeAssertions)
{
	mAllowNegativeAssertions = aAllowNegativeAssertions;
	return(NS_OK);
}

NS_IMETHODIMP
CompositeDataSourceImpl::GetCoalesceDuplicateArcs(PRBool *aCoalesceDuplicateArcs)
{
	*aCoalesceDuplicateArcs = mCoalesceDuplicateArcs;
	return(NS_OK);
}

NS_IMETHODIMP
CompositeDataSourceImpl::SetCoalesceDuplicateArcs(PRBool aCoalesceDuplicateArcs)
{
	mCoalesceDuplicateArcs = aCoalesceDuplicateArcs;
	return(NS_OK);
}

NS_IMETHODIMP
CompositeDataSourceImpl::AddDataSource(nsIRDFDataSource* aDataSource)
{
    NS_ASSERTION(aDataSource != nsnull, "null ptr");
    if (! aDataSource)
        return NS_ERROR_NULL_POINTER;

    mDataSources.AppendObject(aDataSource);
    aDataSource->AddObserver(this);
    return NS_OK;
}



NS_IMETHODIMP
CompositeDataSourceImpl::RemoveDataSource(nsIRDFDataSource* aDataSource)
{
    NS_ASSERTION(aDataSource != nsnull, "null ptr");
    if (! aDataSource)
        return NS_ERROR_NULL_POINTER;


    if (mDataSources.IndexOf(aDataSource) >= 0) {
        aDataSource->RemoveObserver(this);
        mDataSources.RemoveObject(aDataSource);
    }
    return NS_OK;
}


NS_IMETHODIMP
CompositeDataSourceImpl::GetDataSources(nsISimpleEnumerator** _result)
{
    
    
    return NS_NewArrayEnumerator(_result, mDataSources);
}

NS_IMETHODIMP
CompositeDataSourceImpl::OnAssert(nsIRDFDataSource* aDataSource,
                                  nsIRDFResource* aSource,
                                  nsIRDFResource* aProperty,
                                  nsIRDFNode* aTarget)
{
    
    
    
    
    
    
    
	nsresult	rv = NS_OK;

	if (mAllowNegativeAssertions == PR_TRUE)
	{   
		PRBool hasAssertion;
		rv = HasAssertion(aSource, aProperty, aTarget, PR_TRUE, &hasAssertion);
		if (NS_FAILED(rv)) return rv;

		if (! hasAssertion)
			return(NS_OK);
	}

    for (PRInt32 i = mObservers.Count() - 1; i >= 0; --i) {
        mObservers[i]->OnAssert(this, aSource, aProperty, aTarget);
    }
    return NS_OK;
}

NS_IMETHODIMP
CompositeDataSourceImpl::OnUnassert(nsIRDFDataSource* aDataSource,
                                    nsIRDFResource* aSource,
                                    nsIRDFResource* aProperty,
                                    nsIRDFNode* aTarget)
{
    
    
    
    
    
    
    
    nsresult rv;

	if (mAllowNegativeAssertions == PR_TRUE)
	{   
		PRBool hasAssertion;
		rv = HasAssertion(aSource, aProperty, aTarget, PR_TRUE, &hasAssertion);
		if (NS_FAILED(rv)) return rv;

		if (hasAssertion)
			return NS_OK;
	}

    for (PRInt32 i = mObservers.Count() - 1; i >= 0; --i) {
        mObservers[i]->OnUnassert(this, aSource, aProperty, aTarget);
    }
    return NS_OK;
}


NS_IMETHODIMP
CompositeDataSourceImpl::OnChange(nsIRDFDataSource* aDataSource,
                                  nsIRDFResource* aSource,
                                  nsIRDFResource* aProperty,
                                  nsIRDFNode* aOldTarget,
                                  nsIRDFNode* aNewTarget)
{
    
    
    
    
    
    
    for (PRInt32 i = mObservers.Count() - 1; i >= 0; --i) {
        mObservers[i]->OnChange(this, aSource, aProperty,
                                aOldTarget, aNewTarget);
    }
    return NS_OK;
}


NS_IMETHODIMP
CompositeDataSourceImpl::OnMove(nsIRDFDataSource* aDataSource,
                                nsIRDFResource* aOldSource,
                                nsIRDFResource* aNewSource,
                                nsIRDFResource* aProperty,
                                nsIRDFNode* aTarget)
{
    
    
    
    
    
    
    for (PRInt32 i = mObservers.Count() - 1; i >= 0; --i) {
        mObservers[i]->OnMove(this, aOldSource, aNewSource,
                              aProperty, aTarget);
    }
    return NS_OK;
}


NS_IMETHODIMP
CompositeDataSourceImpl::OnBeginUpdateBatch(nsIRDFDataSource* aDataSource)
{
    if (mUpdateBatchNest++ == 0) {
        for (PRInt32 i = mObservers.Count() - 1; i >= 0; --i) {
            mObservers[i]->OnBeginUpdateBatch(this);
        }
    }
    return NS_OK;
}


NS_IMETHODIMP
CompositeDataSourceImpl::OnEndUpdateBatch(nsIRDFDataSource* aDataSource)
{
    NS_ASSERTION(mUpdateBatchNest > 0, "badly nested update batch");
    if (--mUpdateBatchNest == 0) {
        for (PRInt32 i = mObservers.Count() - 1; i >= 0; --i) {
            mObservers[i]->OnEndUpdateBatch(this);
        }
    }
    return NS_OK;
}
