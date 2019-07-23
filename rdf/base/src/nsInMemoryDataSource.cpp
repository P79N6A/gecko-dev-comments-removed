






































































#include "nsAgg.h"
#include "nsCOMPtr.h"
#include "nscore.h"
#include "nsIOutputStream.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFLiteral.h"
#include "nsIRDFNode.h"
#include "nsIRDFObserver.h"
#include "nsIRDFInMemoryDataSource.h"
#include "nsIRDFPropagatableDataSource.h"
#include "nsIRDFPurgeableDataSource.h"
#include "nsIRDFService.h"
#include "nsIServiceManager.h"
#include "nsISupportsArray.h"
#include "nsCOMArray.h"
#include "nsEnumeratorUtils.h"
#include "nsVoidArray.h"  
#include "nsCRT.h"
#include "nsRDFCID.h"
#include "nsRDFBaseDataSources.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsXPIDLString.h"
#include "nsFixedSizeAllocator.h"
#include "rdfutil.h"
#include "pldhash.h"
#include "plstr.h"
#include "prlog.h"
#include "rdf.h"

#include "rdfIDataSource.h"
#include "rdfITripleVisitor.h"

#ifdef PR_LOGGING
static PRLogModuleInfo* gLog = nsnull;
#endif









class Assertion 
{
public:
    static Assertion*
    Create(nsFixedSizeAllocator& aAllocator,
           nsIRDFResource* aSource,
           nsIRDFResource* aProperty,
           nsIRDFNode* aTarget,
           PRBool aTruthValue) {
        void* place = aAllocator.Alloc(sizeof(Assertion));
        return place
            ? ::new (place) Assertion(aSource, aProperty, aTarget, aTruthValue)
            : nsnull; }
    static Assertion*
    Create(nsFixedSizeAllocator& aAllocator, nsIRDFResource* aSource) {
        void* place = aAllocator.Alloc(sizeof(Assertion));
        return place
            ? ::new (place) Assertion(aSource)
            : nsnull; }

    static void
    Destroy(nsFixedSizeAllocator& aAllocator, Assertion* aAssertion) {
        if (aAssertion->mHashEntry && aAssertion->u.hash.mPropertyHash) {
            PL_DHashTableEnumerate(aAssertion->u.hash.mPropertyHash,
                DeletePropertyHashEntry, &aAllocator);
            PL_DHashTableDestroy(aAssertion->u.hash.mPropertyHash);
            aAssertion->u.hash.mPropertyHash = nsnull;
        }
        aAssertion->~Assertion();
        aAllocator.Free(aAssertion, sizeof(*aAssertion)); }

    static PLDHashOperator PR_CALLBACK
    DeletePropertyHashEntry(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
                           PRUint32 aNumber, void* aArg);

    Assertion(nsIRDFResource* aSource,      
              nsIRDFResource* aProperty,
              nsIRDFNode* aTarget,
              PRBool aTruthValue);
    Assertion(nsIRDFResource* aSource);     

    ~Assertion();

    void AddRef() { ++mRefCnt; }

    void Release(nsFixedSizeAllocator& aAllocator) {
        if (--mRefCnt == 0)
            Destroy(aAllocator, this); }

    
    inline  void    Mark()      { u.as.mMarked = PR_TRUE; }
    inline  PRBool  IsMarked()  { return u.as.mMarked; }
    inline  void    Unmark()    { u.as.mMarked = PR_FALSE; }

    

    
    nsIRDFResource*         mSource;
    Assertion*              mNext;

    union
    {
        struct hash
        {
            PLDHashTable*   mPropertyHash; 
        } hash;
        struct as
        {
            nsIRDFResource* mProperty;
            nsIRDFNode*     mTarget;
            Assertion*      mInvNext;
            
            PRPackedBool    mTruthValue;
            PRPackedBool    mMarked;
        } as;
    } u;

    
    
    
    PRInt16                     mRefCnt;
    PRPackedBool                mHashEntry;

private:
    
    
    static void* operator new(size_t) CPP_THROW_NEW { return 0; }
    static void operator delete(void*, size_t) {}
};


struct Entry {
    PLDHashEntryHdr mHdr;
    nsIRDFNode*     mNode;
    Assertion*      mAssertions;
};


Assertion::Assertion(nsIRDFResource* aSource)
    : mSource(aSource),
      mNext(nsnull),
      mRefCnt(0),
      mHashEntry(PR_TRUE)
{
    MOZ_COUNT_CTOR(RDF_Assertion);

    NS_ADDREF(mSource);

    u.hash.mPropertyHash = PL_NewDHashTable(PL_DHashGetStubOps(),
                      nsnull, sizeof(Entry), PL_DHASH_MIN_SIZE);
}

Assertion::Assertion(nsIRDFResource* aSource,
                     nsIRDFResource* aProperty,
                     nsIRDFNode* aTarget,
                     PRBool aTruthValue)
    : mSource(aSource),
      mNext(nsnull),
      mRefCnt(0),
      mHashEntry(PR_FALSE)
{
    MOZ_COUNT_CTOR(RDF_Assertion);

    u.as.mProperty = aProperty;
    u.as.mTarget = aTarget;

    NS_ADDREF(mSource);
    NS_ADDREF(u.as.mProperty);
    NS_ADDREF(u.as.mTarget);

    u.as.mInvNext = nsnull;
    u.as.mTruthValue = aTruthValue;
    u.as.mMarked = PR_FALSE;
}

Assertion::~Assertion()
{
    MOZ_COUNT_DTOR(RDF_Assertion);
#ifdef DEBUG_REFS
    --gInstanceCount;
    fprintf(stdout, "%d - RDF: Assertion\n", gInstanceCount);
#endif

    NS_RELEASE(mSource);
    if (!mHashEntry)
    {
        NS_RELEASE(u.as.mProperty);
        NS_RELEASE(u.as.mTarget);
    }
}

PLDHashOperator PR_CALLBACK
Assertion::DeletePropertyHashEntry(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
                                           PRUint32 aNumber, void* aArg)
{
    Entry* entry = NS_REINTERPRET_CAST(Entry*, aHdr);
    nsFixedSizeAllocator* allocator = NS_STATIC_CAST(nsFixedSizeAllocator*, aArg);

    Assertion* as = entry->mAssertions;
    while (as) {
        Assertion* doomed = as;
        as = as->mNext;

        
        doomed->mNext = doomed->u.as.mInvNext = nsnull;
        doomed->Release(*allocator);
    }
    return PL_DHASH_NEXT;
}





class InMemoryArcsEnumeratorImpl;
class InMemoryAssertionEnumeratorImpl;
class InMemoryResourceEnumeratorImpl;

class InMemoryDataSource : public nsIRDFDataSource,
                           public nsIRDFInMemoryDataSource,
                           public nsIRDFPropagatableDataSource,
                           public nsIRDFPurgeableDataSource,
                           public rdfIDataSource
{
protected:
    nsFixedSizeAllocator mAllocator;

    
    
    
    
    
    PLDHashTable mForwardArcs; 
    PLDHashTable mReverseArcs; 

    nsCOMArray<nsIRDFObserver> mObservers;  
    PRUint32                   mNumObservers;

    
    
    PRUint32 mReadCount;

    static PLDHashOperator PR_CALLBACK
    DeleteForwardArcsEntry(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
                           PRUint32 aNumber, void* aArg);

    static PLDHashOperator PR_CALLBACK
    ResourceEnumerator(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
                       PRUint32 aNumber, void* aArg);

    friend class InMemoryArcsEnumeratorImpl;
    friend class InMemoryAssertionEnumeratorImpl;
    friend class InMemoryResourceEnumeratorImpl; 

    
    nsresult
    LockedAssert(nsIRDFResource* source, 
                 nsIRDFResource* property, 
                 nsIRDFNode* target,
                 PRBool tv);

    nsresult
    LockedUnassert(nsIRDFResource* source,
                   nsIRDFResource* property,
                   nsIRDFNode* target);

    InMemoryDataSource(nsISupports* aOuter);
    virtual ~InMemoryDataSource();
    nsresult Init();

    friend NS_IMETHODIMP
    NS_NewRDFInMemoryDataSource(nsISupports* aOuter, const nsIID& aIID, void** aResult);

public:
    NS_DECL_CYCLE_COLLECTING_AGGREGATED
    NS_DECL_AGGREGATED_CYCLE_COLLECTION_CLASS(InMemoryDataSource)

    
    NS_DECL_NSIRDFDATASOURCE

    
    NS_DECL_NSIRDFINMEMORYDATASOURCE

    
    NS_DECL_NSIRDFPROPAGATABLEDATASOURCE

    
    NS_DECL_NSIRDFPURGEABLEDATASOURCE

    
    NS_DECL_RDFIDATASOURCE

protected:
    static PLDHashOperator PR_CALLBACK
    SweepForwardArcsEntries(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
                            PRUint32 aNumber, void* aArg);

public:
    
    Assertion*
    GetForwardArcs(nsIRDFResource* u) {
        PLDHashEntryHdr* hdr = PL_DHashTableOperate(&mForwardArcs, u, PL_DHASH_LOOKUP);
        return PL_DHASH_ENTRY_IS_BUSY(hdr)
            ? NS_REINTERPRET_CAST(Entry*, hdr)->mAssertions
            : nsnull; }

    Assertion*
    GetReverseArcs(nsIRDFNode* v) {
        PLDHashEntryHdr* hdr = PL_DHashTableOperate(&mReverseArcs, v, PL_DHASH_LOOKUP);
        return PL_DHASH_ENTRY_IS_BUSY(hdr)
            ? NS_REINTERPRET_CAST(Entry*, hdr)->mAssertions
            : nsnull; }

    void
    SetForwardArcs(nsIRDFResource* u, Assertion* as) {
        PLDHashEntryHdr* hdr = PL_DHashTableOperate(&mForwardArcs, u,
                                                    as ? PL_DHASH_ADD : PL_DHASH_REMOVE);
        if (as && hdr) {
            Entry* entry = NS_REINTERPRET_CAST(Entry*, hdr);
            entry->mNode = u;
            entry->mAssertions = as;
        } }

    void
    SetReverseArcs(nsIRDFNode* v, Assertion* as) {
        PLDHashEntryHdr* hdr = PL_DHashTableOperate(&mReverseArcs, v,
                                                    as ? PL_DHASH_ADD : PL_DHASH_REMOVE);
        if (as && hdr) {
            Entry* entry = NS_REINTERPRET_CAST(Entry*, hdr);
            entry->mNode = v;
            entry->mAssertions = as;
        } }

#ifdef PR_LOGGING
    void
    LogOperation(const char* aOperation,
                 nsIRDFResource* asource,
                 nsIRDFResource* aProperty,
                 nsIRDFNode* aTarget,
                 PRBool aTruthValue = PR_TRUE);
#endif

    PRBool  mPropagateChanges;
};









class InMemoryAssertionEnumeratorImpl : public nsISimpleEnumerator
{
private:
    InMemoryDataSource* mDataSource;
    nsIRDFResource* mSource;
    nsIRDFResource* mProperty;
    nsIRDFNode*     mTarget;
    nsIRDFNode*     mValue;
    PRInt32         mCount;
    PRBool          mTruthValue;
    Assertion*      mNextAssertion;
    nsCOMPtr<nsISupportsArray> mHashArcs;

    
    
    static void* operator new(size_t) CPP_THROW_NEW { return 0; }
    static void operator delete(void*, size_t) {}

    InMemoryAssertionEnumeratorImpl(InMemoryDataSource* aDataSource,
                                    nsIRDFResource* aSource,
                                    nsIRDFResource* aProperty,
                                    nsIRDFNode* aTarget,
                                    PRBool aTruthValue);

    virtual ~InMemoryAssertionEnumeratorImpl();

public:
    static InMemoryAssertionEnumeratorImpl*
    Create(InMemoryDataSource* aDataSource,
           nsIRDFResource* aSource,
           nsIRDFResource* aProperty,
           nsIRDFNode* aTarget,
           PRBool aTruthValue) {
        void* place = aDataSource->mAllocator.Alloc(sizeof(InMemoryAssertionEnumeratorImpl));
        return place
            ? ::new (place) InMemoryAssertionEnumeratorImpl(aDataSource,
                                                            aSource, aProperty, aTarget,
                                                            aTruthValue)
            : nsnull; }

    static void
    Destroy(InMemoryAssertionEnumeratorImpl* aEnumerator) {
        
        
        nsCOMPtr<nsIRDFDataSource> kungFuDeathGrip = aEnumerator->mDataSource;

        
        
        nsFixedSizeAllocator& pool = aEnumerator->mDataSource->mAllocator;
        aEnumerator->~InMemoryAssertionEnumeratorImpl();
        pool.Free(aEnumerator, sizeof(*aEnumerator)); }

    
    NS_DECL_ISUPPORTS
   
    
    NS_DECL_NSISIMPLEENUMERATOR
};




InMemoryAssertionEnumeratorImpl::InMemoryAssertionEnumeratorImpl(
                 InMemoryDataSource* aDataSource,
                 nsIRDFResource* aSource,
                 nsIRDFResource* aProperty,
                 nsIRDFNode* aTarget,
                 PRBool aTruthValue)
    : mDataSource(aDataSource),
      mSource(aSource),
      mProperty(aProperty),
      mTarget(aTarget),
      mValue(nsnull),
      mCount(0),
      mTruthValue(aTruthValue),
      mNextAssertion(nsnull)
{
    NS_ADDREF(mDataSource);
    NS_IF_ADDREF(mSource);
    NS_ADDREF(mProperty);
    NS_IF_ADDREF(mTarget);

    if (mSource) {
        mNextAssertion = mDataSource->GetForwardArcs(mSource);

        if (mNextAssertion && mNextAssertion->mHashEntry) {
            
            PLDHashEntryHdr* hdr = PL_DHashTableOperate(mNextAssertion->u.hash.mPropertyHash,
                aProperty, PL_DHASH_LOOKUP);
            mNextAssertion = PL_DHASH_ENTRY_IS_BUSY(hdr)
                ? NS_REINTERPRET_CAST(Entry*, hdr)->mAssertions
                : nsnull;
        }
    }
    else {
        mNextAssertion = mDataSource->GetReverseArcs(mTarget);
    }

    
    if (mNextAssertion)
        mNextAssertion->AddRef();
}

InMemoryAssertionEnumeratorImpl::~InMemoryAssertionEnumeratorImpl()
{
#ifdef DEBUG_REFS
    --gInstanceCount;
    fprintf(stdout, "%d - RDF: InMemoryAssertionEnumeratorImpl\n", gInstanceCount);
#endif

    if (mNextAssertion)
        mNextAssertion->Release(mDataSource->mAllocator);

    NS_IF_RELEASE(mDataSource);
    NS_IF_RELEASE(mSource);
    NS_IF_RELEASE(mProperty);
    NS_IF_RELEASE(mTarget);
    NS_IF_RELEASE(mValue);
}

NS_IMPL_ADDREF(InMemoryAssertionEnumeratorImpl)
NS_IMPL_RELEASE_WITH_DESTROY(InMemoryAssertionEnumeratorImpl, Destroy(this))
NS_IMPL_QUERY_INTERFACE1(InMemoryAssertionEnumeratorImpl, nsISimpleEnumerator)

NS_IMETHODIMP
InMemoryAssertionEnumeratorImpl::HasMoreElements(PRBool* aResult)
{
    if (mValue) {
        *aResult = PR_TRUE;
        return NS_OK;
    }

    while (mNextAssertion) {
        PRBool foundIt = PR_FALSE;
        if ((mProperty == mNextAssertion->u.as.mProperty) &&
            (mTruthValue == mNextAssertion->u.as.mTruthValue)) {
            if (mSource) {
                mValue = mNextAssertion->u.as.mTarget;
                NS_ADDREF(mValue);
            }
            else {
                mValue = mNextAssertion->mSource;
                NS_ADDREF(mValue);
            }
            foundIt = PR_TRUE;
        }

        
        Assertion* as = mNextAssertion;

        
        mNextAssertion = (mSource) ? mNextAssertion->mNext : mNextAssertion->u.as.mInvNext;

        
        if (mNextAssertion)
            mNextAssertion->AddRef();

        
        as->Release(mDataSource->mAllocator);

        if (foundIt) {
            *aResult = PR_TRUE;
            return NS_OK;
        }
    }

    *aResult = PR_FALSE;
    return NS_OK;
}


NS_IMETHODIMP
InMemoryAssertionEnumeratorImpl::GetNext(nsISupports** aResult)
{
    nsresult rv;

    PRBool hasMore;
    rv = HasMoreElements(&hasMore);
    if (NS_FAILED(rv)) return rv;

    if (! hasMore)
        return NS_ERROR_UNEXPECTED;

    
    *aResult = mValue;
    mValue = nsnull;

    return NS_OK;
}












class InMemoryArcsEnumeratorImpl : public nsISimpleEnumerator
{
private:
    
    
    static void* operator new(size_t) CPP_THROW_NEW { return 0; }
    static void operator delete(void*, size_t) {}

    InMemoryDataSource* mDataSource;
    nsIRDFResource*     mSource;
    nsIRDFNode*         mTarget;
    nsAutoVoidArray     mAlreadyReturned;
    nsIRDFResource*     mCurrent;
    Assertion*          mAssertion;
    nsCOMPtr<nsISupportsArray> mHashArcs;

    InMemoryArcsEnumeratorImpl(InMemoryDataSource* aDataSource,
                               nsIRDFResource* aSource,
                               nsIRDFNode* aTarget);

    virtual ~InMemoryArcsEnumeratorImpl();

    static PLDHashOperator PR_CALLBACK
    ArcEnumerator(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
                       PRUint32 aNumber, void* aArg);

public:
    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSISIMPLEENUMERATOR

    static InMemoryArcsEnumeratorImpl*
    Create(InMemoryDataSource* aDataSource,
           nsIRDFResource* aSource,
           nsIRDFNode* aTarget) {
        void* place = aDataSource->mAllocator.Alloc(sizeof(InMemoryArcsEnumeratorImpl));
        return place
            ? ::new (place) InMemoryArcsEnumeratorImpl(aDataSource, aSource, aTarget)
            : nsnull; }

    static void
    Destroy(InMemoryArcsEnumeratorImpl* aEnumerator) {
        
        
        nsCOMPtr<nsIRDFDataSource> kungFuDeathGrip = aEnumerator->mDataSource;

        
        
        nsFixedSizeAllocator& pool = aEnumerator->mDataSource->mAllocator;
        aEnumerator->~InMemoryArcsEnumeratorImpl();
        pool.Free(aEnumerator, sizeof(*aEnumerator)); }
};


PLDHashOperator PR_CALLBACK
InMemoryArcsEnumeratorImpl::ArcEnumerator(PLDHashTable* aTable,
                                       PLDHashEntryHdr* aHdr,
                                       PRUint32 aNumber, void* aArg)
{
    Entry* entry = NS_REINTERPRET_CAST(Entry*, aHdr);
    nsISupportsArray* resources = NS_STATIC_CAST(nsISupportsArray*, aArg);

    resources->AppendElement(entry->mNode);
    return PL_DHASH_NEXT;
}


InMemoryArcsEnumeratorImpl::InMemoryArcsEnumeratorImpl(InMemoryDataSource* aDataSource,
                                                       nsIRDFResource* aSource,
                                                       nsIRDFNode* aTarget)
    : mDataSource(aDataSource),
      mSource(aSource),
      mTarget(aTarget),
      mCurrent(nsnull)
{
    NS_ADDREF(mDataSource);
    NS_IF_ADDREF(mSource);
    NS_IF_ADDREF(mTarget);

    if (mSource) {
        
        mAssertion = mDataSource->GetForwardArcs(mSource);

        if (mAssertion && mAssertion->mHashEntry) {
            
            nsresult rv = NS_NewISupportsArray(getter_AddRefs(mHashArcs));
            if (NS_SUCCEEDED(rv)) {
                PL_DHashTableEnumerate(mAssertion->u.hash.mPropertyHash,
                    ArcEnumerator, mHashArcs.get());
            }
            mAssertion = nsnull;
        }
    }
    else {
        mAssertion = mDataSource->GetReverseArcs(mTarget);
    }
}

InMemoryArcsEnumeratorImpl::~InMemoryArcsEnumeratorImpl()
{
#ifdef DEBUG_REFS
    --gInstanceCount;
    fprintf(stdout, "%d - RDF: InMemoryArcsEnumeratorImpl\n", gInstanceCount);
#endif

    NS_RELEASE(mDataSource);
    NS_IF_RELEASE(mSource);
    NS_IF_RELEASE(mTarget);
    NS_IF_RELEASE(mCurrent);

    for (PRInt32 i = mAlreadyReturned.Count() - 1; i >= 0; --i) {
        nsIRDFResource* resource = (nsIRDFResource*) mAlreadyReturned[i];
        NS_RELEASE(resource);
    }
}

NS_IMPL_ADDREF(InMemoryArcsEnumeratorImpl)
NS_IMPL_RELEASE_WITH_DESTROY(InMemoryArcsEnumeratorImpl, Destroy(this))
NS_IMPL_QUERY_INTERFACE1(InMemoryArcsEnumeratorImpl, nsISimpleEnumerator)

NS_IMETHODIMP
InMemoryArcsEnumeratorImpl::HasMoreElements(PRBool* aResult)
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    if (mCurrent) {
        *aResult = PR_TRUE;
        return NS_OK;
    }

    if (mHashArcs) {
        PRUint32    itemCount;
        nsresult    rv;
        if (NS_FAILED(rv = mHashArcs->Count(&itemCount)))   return(rv);
        if (itemCount > 0) {
            --itemCount;
            mCurrent = NS_STATIC_CAST(nsIRDFResource *,
                                      mHashArcs->ElementAt(itemCount));
            mHashArcs->RemoveElementAt(itemCount);
            *aResult = PR_TRUE;
            return NS_OK;
        }
    }
    else
        while (mAssertion) {
            nsIRDFResource* next = mAssertion->u.as.mProperty;

            
            
            
            
            
            
            
            
            
            
            
            

            do {
                mAssertion = (mSource ? mAssertion->mNext :
                        mAssertion->u.as.mInvNext);
            }
            while (mAssertion && (next == mAssertion->u.as.mProperty));

            PRBool alreadyReturned = PR_FALSE;
            for (PRInt32 i = mAlreadyReturned.Count() - 1; i >= 0; --i) {
                if (mAlreadyReturned[i] == next) {
                    alreadyReturned = PR_TRUE;
                    break;
                }
            }

            if (! alreadyReturned) {
                mCurrent = next;
                NS_ADDREF(mCurrent);
                *aResult = PR_TRUE;
                return NS_OK;
            }
        }

    *aResult = PR_FALSE;
    return NS_OK;
}


NS_IMETHODIMP
InMemoryArcsEnumeratorImpl::GetNext(nsISupports** aResult)
{
    nsresult rv;

    PRBool hasMore;
    rv = HasMoreElements(&hasMore);
    if (NS_FAILED(rv)) return rv;

    if (! hasMore)
        return NS_ERROR_UNEXPECTED;

    
    
    NS_ADDREF(mCurrent);
    mAlreadyReturned.AppendElement(mCurrent);

    
    *aResult = mCurrent;
    mCurrent = nsnull;

    return NS_OK;
}





NS_IMETHODIMP
NS_NewRDFInMemoryDataSource(nsISupports* aOuter, const nsIID& aIID, void** aResult)
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;
    *aResult = nsnull;

    if (aOuter && !aIID.Equals(NS_GET_IID(nsISupports))) {
        NS_ERROR("aggregation requires nsISupports");
        return NS_ERROR_ILLEGAL_VALUE;
    }

    InMemoryDataSource* datasource = new InMemoryDataSource(aOuter);
    if (! datasource)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(datasource);

    nsresult rv = datasource->Init();
    if (NS_SUCCEEDED(rv)) {
        datasource->fAggregated.AddRef();
        rv = datasource->AggregatedQueryInterface(aIID, aResult); 
        datasource->fAggregated.Release();
    }

    NS_RELEASE(datasource);
    return rv;
}


InMemoryDataSource::InMemoryDataSource(nsISupports* aOuter)
    : mNumObservers(0), mReadCount(0)
{
    NS_INIT_AGGREGATED(aOuter);

    static const size_t kBucketSizes[] = {
        sizeof(Assertion),
        sizeof(Entry),
        sizeof(InMemoryArcsEnumeratorImpl),
        sizeof(InMemoryAssertionEnumeratorImpl) };

    static const PRInt32 kNumBuckets = sizeof(kBucketSizes) / sizeof(size_t);

    
    static const PRInt32 kInitialSize = 1024;

    mAllocator.Init("nsInMemoryDataSource", kBucketSizes, kNumBuckets, kInitialSize);

    mForwardArcs.ops = nsnull;
    mReverseArcs.ops = nsnull;
    mPropagateChanges = PR_TRUE;
}


nsresult
InMemoryDataSource::Init()
{
    if (!PL_DHashTableInit(&mForwardArcs,
                           PL_DHashGetStubOps(),
                           nsnull,
                           sizeof(Entry),
                           PL_DHASH_MIN_SIZE)) {
        mForwardArcs.ops = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
    }
    if (!PL_DHashTableInit(&mReverseArcs,
                           PL_DHashGetStubOps(),
                           nsnull,
                           sizeof(Entry),
                           PL_DHASH_MIN_SIZE)) {
        mReverseArcs.ops = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
    }

#ifdef PR_LOGGING
    if (! gLog)
        gLog = PR_NewLogModule("InMemoryDataSource");
#endif

    return NS_OK;
}


InMemoryDataSource::~InMemoryDataSource()
{
#ifdef DEBUG_REFS
    --gInstanceCount;
    fprintf(stdout, "%d - RDF: InMemoryDataSource\n", gInstanceCount);
#endif

    if (mForwardArcs.ops) {
        
        
        
        
        PL_DHashTableEnumerate(&mForwardArcs, DeleteForwardArcsEntry, &mAllocator);
        PL_DHashTableFinish(&mForwardArcs);
    }
    if (mReverseArcs.ops)
        PL_DHashTableFinish(&mReverseArcs);

    PR_LOG(gLog, PR_LOG_NOTICE,
           ("InMemoryDataSource(%p): destroyed.", this));

}

PLDHashOperator PR_CALLBACK
InMemoryDataSource::DeleteForwardArcsEntry(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
                                           PRUint32 aNumber, void* aArg)
{
    Entry* entry = NS_REINTERPRET_CAST(Entry*, aHdr);
    nsFixedSizeAllocator* allocator = NS_STATIC_CAST(nsFixedSizeAllocator*, aArg);

    Assertion* as = entry->mAssertions;
    while (as) {
        Assertion* doomed = as;
        as = as->mNext;

        
        doomed->mNext = doomed->u.as.mInvNext = nsnull;
        doomed->Release(*allocator);
    }
    return PL_DHASH_NEXT;
}




NS_IMPL_CYCLE_COLLECTION_CLASS(InMemoryDataSource)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(InMemoryDataSource)
    NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mObservers)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_AGGREGATED(InMemoryDataSource)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mObservers)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_AGGREGATED(InMemoryDataSource)
NS_INTERFACE_MAP_BEGIN_AGGREGATED(InMemoryDataSource)
    NS_INTERFACE_MAP_ENTRY(nsIRDFDataSource)
    NS_INTERFACE_MAP_ENTRY(nsIRDFInMemoryDataSource)
    NS_INTERFACE_MAP_ENTRY(nsIRDFPropagatableDataSource)
    NS_INTERFACE_MAP_ENTRY(nsIRDFPurgeableDataSource)
    NS_INTERFACE_MAP_ENTRY(rdfIDataSource)
    NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION_AGGREGATED(InMemoryDataSource)
NS_INTERFACE_MAP_END




#ifdef PR_LOGGING
void
InMemoryDataSource::LogOperation(const char* aOperation,
                                 nsIRDFResource* aSource,
                                 nsIRDFResource* aProperty,
                                 nsIRDFNode* aTarget,
                                 PRBool aTruthValue)
{
    if (! PR_LOG_TEST(gLog, PR_LOG_NOTICE))
        return;

    nsXPIDLCString uri;
    aSource->GetValue(getter_Copies(uri));
    PR_LogPrint
           ("InMemoryDataSource(%p): %s", this, aOperation);

    PR_LogPrint
           ("  [(%p)%s]--", aSource, (const char*) uri);

    aProperty->GetValue(getter_Copies(uri));

    char tv = (aTruthValue ? '-' : '!');
    PR_LogPrint
           ("  --%c[(%p)%s]--", tv, aProperty, (const char*) uri);

    nsCOMPtr<nsIRDFResource> resource;
    nsCOMPtr<nsIRDFLiteral> literal;

    if ((resource = do_QueryInterface(aTarget)) != nsnull) {
        resource->GetValue(getter_Copies(uri));
        PR_LogPrint
           ("  -->[(%p)%s]", aTarget, (const char*) uri);
    }
    else if ((literal = do_QueryInterface(aTarget)) != nsnull) {
        nsXPIDLString value;
        literal->GetValue(getter_Copies(value));
        nsAutoString valueStr(value);
        char* valueCStr = ToNewCString(valueStr);

        PR_LogPrint
           ("  -->(\"%s\")\n", valueCStr);

        NS_Free(valueCStr);
    }
    else {
        PR_LogPrint
           ("  -->(unknown-type)\n");
    }
}
#endif


NS_IMETHODIMP
InMemoryDataSource::GetURI(char* *uri)
{
    NS_PRECONDITION(uri != nsnull, "null ptr");
    if (! uri)
        return NS_ERROR_NULL_POINTER;

    *uri = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
InMemoryDataSource::GetSource(nsIRDFResource* property,
                              nsIRDFNode* target,
                              PRBool tv,
                              nsIRDFResource** source)
{
    NS_PRECONDITION(source != nsnull, "null ptr");
    if (! source)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(property != nsnull, "null ptr");
    if (! property)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(target != nsnull, "null ptr");
    if (! target)
        return NS_ERROR_NULL_POINTER;

    for (Assertion* as = GetReverseArcs(target); as; as = as->u.as.mInvNext) {
        if ((property == as->u.as.mProperty) && (tv == as->u.as.mTruthValue)) {
            *source = as->mSource;
            NS_ADDREF(*source);
            return NS_OK;
        }
    }
    *source = nsnull;
    return NS_RDF_NO_VALUE;
}

NS_IMETHODIMP
InMemoryDataSource::GetTarget(nsIRDFResource* source,
                              nsIRDFResource* property,
                              PRBool tv,
                              nsIRDFNode** target)
{
    NS_PRECONDITION(source != nsnull, "null ptr");
    if (! source)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(property != nsnull, "null ptr");
    if (! property)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(target != nsnull, "null ptr");
    if (! target)
        return NS_ERROR_NULL_POINTER;

    Assertion *as = GetForwardArcs(source);
    if (as && as->mHashEntry) {
        PLDHashEntryHdr* hdr = PL_DHashTableOperate(as->u.hash.mPropertyHash, property, PL_DHASH_LOOKUP);
        Assertion* val = PL_DHASH_ENTRY_IS_BUSY(hdr)
            ? NS_REINTERPRET_CAST(Entry*, hdr)->mAssertions
            : nsnull;
        while (val) {
            if (tv == val->u.as.mTruthValue) {
                *target = val->u.as.mTarget;
                NS_IF_ADDREF(*target);
                return NS_OK;
            }
            val = val->mNext;
        }
    }
    else
    for (; as != nsnull; as = as->mNext) {
        if ((property == as->u.as.mProperty) && (tv == (as->u.as.mTruthValue))) {
            *target = as->u.as.mTarget;
            NS_ADDREF(*target);
            return NS_OK;
        }
    }

    
    
    *target = nsnull;
    return NS_RDF_NO_VALUE;
}

NS_IMETHODIMP
InMemoryDataSource::HasAssertion(nsIRDFResource* source,
                                 nsIRDFResource* property,
                                 nsIRDFNode* target,
                                 PRBool tv,
                                 PRBool* hasAssertion)
{
    if (! source)
        return NS_ERROR_NULL_POINTER;

    if (! property)
        return NS_ERROR_NULL_POINTER;

    if (! target)
        return NS_ERROR_NULL_POINTER;

    Assertion *as = GetForwardArcs(source);
    if (as && as->mHashEntry) {
        PLDHashEntryHdr* hdr = PL_DHashTableOperate(as->u.hash.mPropertyHash, property, PL_DHASH_LOOKUP);
        Assertion* val = PL_DHASH_ENTRY_IS_BUSY(hdr)
            ? NS_REINTERPRET_CAST(Entry*, hdr)->mAssertions
            : nsnull;
        while (val) {
            if ((val->u.as.mTarget == target) && (tv == (val->u.as.mTruthValue))) {
                *hasAssertion = PR_TRUE;
                return NS_OK;
            }
            val = val->mNext;
        }
    }
    else
    for (; as != nsnull; as = as->mNext) {
        
        if (target != as->u.as.mTarget)
            continue;

        if (property != as->u.as.mProperty)
            continue;

        if (tv != (as->u.as.mTruthValue))
            continue;

        
        *hasAssertion = PR_TRUE;
        return NS_OK;
    }

    
    *hasAssertion = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
InMemoryDataSource::GetSources(nsIRDFResource* aProperty,
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

    InMemoryAssertionEnumeratorImpl* result =
        InMemoryAssertionEnumeratorImpl::Create(this, nsnull, aProperty,
                                                  aTarget, aTruthValue);

    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(result);
    *aResult = result;

    return NS_OK;
}

NS_IMETHODIMP
InMemoryDataSource::GetTargets(nsIRDFResource* aSource,
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

    InMemoryAssertionEnumeratorImpl* result =
        InMemoryAssertionEnumeratorImpl::Create(this, aSource, aProperty,
                                                nsnull, aTruthValue);

    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(result);
    *aResult = result;

    return NS_OK;
}


nsresult
InMemoryDataSource::LockedAssert(nsIRDFResource* aSource,
                                 nsIRDFResource* aProperty,
                                 nsIRDFNode* aTarget,
                                 PRBool aTruthValue)
{
#ifdef PR_LOGGING
    LogOperation("ASSERT", aSource, aProperty, aTarget, aTruthValue);
#endif

    Assertion* next = GetForwardArcs(aSource);
    Assertion* prev = next;
    Assertion* as = nsnull;

    PRBool  haveHash = (next) ? next->mHashEntry : PR_FALSE;
    if (haveHash) {
        PLDHashEntryHdr* hdr = PL_DHashTableOperate(next->u.hash.mPropertyHash, aProperty, PL_DHASH_LOOKUP);
        Assertion* val = PL_DHASH_ENTRY_IS_BUSY(hdr)
            ? NS_REINTERPRET_CAST(Entry*, hdr)->mAssertions
            : nsnull;
        while (val) {
            if (val->u.as.mTarget == aTarget) {
                
                
                val->u.as.mTruthValue = aTruthValue;
                return NS_OK;
            }
            val = val->mNext;
        }
    }
    else
    {
        while (next) {
            
            if (aTarget == next->u.as.mTarget) {
                if (aProperty == next->u.as.mProperty) {
                    
                    
                    next->u.as.mTruthValue = aTruthValue;
                    return NS_OK;
                }
            }

            prev = next;
            next = next->mNext;
        }
    }

    as = Assertion::Create(mAllocator, aSource, aProperty, aTarget, aTruthValue);
    if (! as)
        return NS_ERROR_OUT_OF_MEMORY;

    
    as->AddRef();

    if (haveHash)
    {
        PLDHashEntryHdr* hdr = PL_DHashTableOperate(next->u.hash.mPropertyHash,
            aProperty, PL_DHASH_LOOKUP);
        Assertion *asRef = PL_DHASH_ENTRY_IS_BUSY(hdr)
            ? NS_REINTERPRET_CAST(Entry*, hdr)->mAssertions
            : nsnull;
        if (asRef)
        {
            as->mNext = asRef->mNext;
            asRef->mNext = as;
        }
        else
        {
            hdr = PL_DHashTableOperate(next->u.hash.mPropertyHash,
                                            aProperty, PL_DHASH_ADD);
            if (hdr)
            {
                Entry* entry = NS_REINTERPRET_CAST(Entry*, hdr);
                entry->mNode = aProperty;
                entry->mAssertions = as;
            }
        }
    }
    else
    {
        
        if (!prev) {
            SetForwardArcs(aSource, as);
        } else {
            prev->mNext = as;
        }
    }

    

    next = GetReverseArcs(aTarget);
    as->u.as.mInvNext = next;
    next = as;
    SetReverseArcs(aTarget, next);

    return NS_OK;
}

NS_IMETHODIMP
InMemoryDataSource::Assert(nsIRDFResource* aSource,
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

    if (mReadCount) {
        NS_WARNING("Writing to InMemoryDataSource during read\n");
        return NS_RDF_ASSERTION_REJECTED;
    }

    nsresult rv;
    rv = LockedAssert(aSource, aProperty, aTarget, aTruthValue);
    if (NS_FAILED(rv)) return rv;

    
    for (PRInt32 i = (PRInt32)mNumObservers - 1; mPropagateChanges && i >= 0; --i) {
        nsIRDFObserver* obs = mObservers[i];

        
        NS_ASSERTION(obs, "observer array corrupted!");
        if (! obs)
          continue;

        obs->OnAssert(this, aSource, aProperty, aTarget);
        
    }

    return NS_RDF_ASSERTION_ACCEPTED;
}


nsresult
InMemoryDataSource::LockedUnassert(nsIRDFResource* aSource,
                                   nsIRDFResource* aProperty,
                                   nsIRDFNode* aTarget)
{
#ifdef PR_LOGGING
    LogOperation("UNASSERT", aSource, aProperty, aTarget);
#endif

    Assertion* next = GetForwardArcs(aSource);
    Assertion* prev = next;
    Assertion* root = next;
    Assertion* as = nsnull;
    
    PRBool  haveHash = (next) ? next->mHashEntry : PR_FALSE;
    if (haveHash) {
        PLDHashEntryHdr* hdr = PL_DHashTableOperate(next->u.hash.mPropertyHash,
            aProperty, PL_DHASH_LOOKUP);
        prev = next = PL_DHASH_ENTRY_IS_BUSY(hdr)
            ? NS_REINTERPRET_CAST(Entry*, hdr)->mAssertions
            : nsnull;
        PRBool first = PR_TRUE;
        while (next) {
            if (aTarget == next->u.as.mTarget) {
                break;
            }
            first = PR_FALSE;
            prev = next;
            next = next->mNext;
        }
        
        if (!next)
            return NS_OK;

        as = next;

        if (first) {
            PL_DHashTableRawRemove(root->u.hash.mPropertyHash, hdr);

            if (next && next->mNext) {
                PLDHashEntryHdr* hdr = PL_DHashTableOperate(root->u.hash.mPropertyHash,
                                     aProperty, PL_DHASH_ADD);
                if (hdr) {
                    Entry* entry = NS_REINTERPRET_CAST(Entry*, hdr);
                    entry->mNode = aProperty;
                    entry->mAssertions = next->mNext;
                }
            }
            else {
                
                if (!root->u.hash.mPropertyHash->entryCount) {
                    Assertion::Destroy(mAllocator, root);
                    SetForwardArcs(aSource, nsnull);
                }
            }
        }
        else {
            prev->mNext = next->mNext;
        }
    }
    else
    {
        while (next) {
            
            if (aTarget == next->u.as.mTarget) {
                if (aProperty == next->u.as.mProperty) {
                    if (prev == next) {
                        SetForwardArcs(aSource, next->mNext);
                    } else {
                        prev->mNext = next->mNext;
                    }
                    as = next;
                    break;
                }
            }

            prev = next;
            next = next->mNext;
        }
    }
    
    if (!as)
        return NS_OK;

#ifdef DEBUG
    PRBool foundReverseArc = PR_FALSE;
#endif

    next = prev = GetReverseArcs(aTarget);
    while (next) {
        if (next == as) {
            if (prev == next) {
                SetReverseArcs(aTarget, next->u.as.mInvNext);
            } else {
                prev->u.as.mInvNext = next->u.as.mInvNext;
            }
#ifdef DEBUG
            foundReverseArc = PR_TRUE;
#endif
            break;
        }
        prev = next;
        next = next->u.as.mInvNext;
    }

#ifdef DEBUG
    NS_ASSERTION(foundReverseArc, "in-memory db corrupted: unable to find reverse arc");
#endif

    
    as->mNext = as->u.as.mInvNext = nsnull;
    as->Release(mAllocator);

    return NS_OK;
}

NS_IMETHODIMP
InMemoryDataSource::Unassert(nsIRDFResource* aSource,
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

    if (mReadCount) {
        NS_WARNING("Writing to InMemoryDataSource during read\n");
        return NS_RDF_ASSERTION_REJECTED;
    }

    nsresult rv;

    rv = LockedUnassert(aSource, aProperty, aTarget);
    if (NS_FAILED(rv)) return rv;

    
    for (PRInt32 i = PRInt32(mNumObservers) - 1; mPropagateChanges && i >= 0; --i) {
        nsIRDFObserver* obs = mObservers[i];

        
        NS_ASSERTION(obs, "observer array corrupted!");
        if (! obs)
          continue;

        obs->OnUnassert(this, aSource, aProperty, aTarget);
        
    }

    return NS_RDF_ASSERTION_ACCEPTED;
}


NS_IMETHODIMP
InMemoryDataSource::Change(nsIRDFResource* aSource,
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

    if (mReadCount) {
        NS_WARNING("Writing to InMemoryDataSource during read\n");
        return NS_RDF_ASSERTION_REJECTED;
    }

    nsresult rv;

    
    

    rv = LockedUnassert(aSource, aProperty, aOldTarget);
    if (NS_FAILED(rv)) return rv;

    rv = LockedAssert(aSource, aProperty, aNewTarget, PR_TRUE);
    if (NS_FAILED(rv)) return rv;

    
    for (PRInt32 i = PRInt32(mNumObservers) - 1; mPropagateChanges && i >= 0; --i) {
        nsIRDFObserver* obs = mObservers[i];

        
        NS_ASSERTION(obs, "observer array corrupted!");
        if (! obs)
          continue;

        obs->OnChange(this, aSource, aProperty, aOldTarget, aNewTarget);
        
    }

    return NS_RDF_ASSERTION_ACCEPTED;
}


NS_IMETHODIMP
InMemoryDataSource::Move(nsIRDFResource* aOldSource,
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

    if (mReadCount) {
        NS_WARNING("Writing to InMemoryDataSource during read\n");
        return NS_RDF_ASSERTION_REJECTED;
    }

    nsresult rv;

    
    

    rv = LockedUnassert(aOldSource, aProperty, aTarget);
    if (NS_FAILED(rv)) return rv;

    rv = LockedAssert(aNewSource, aProperty, aTarget, PR_TRUE);
    if (NS_FAILED(rv)) return rv;

    
    for (PRInt32 i = PRInt32(mNumObservers) - 1; mPropagateChanges && i >= 0; --i) {
        nsIRDFObserver* obs = mObservers[i];

        
        NS_ASSERTION(obs, "observer array corrupted!");
        if (! obs)
          continue;

        obs->OnMove(this, aOldSource, aNewSource, aProperty, aTarget);
        
    }

    return NS_RDF_ASSERTION_ACCEPTED;
}


NS_IMETHODIMP
InMemoryDataSource::AddObserver(nsIRDFObserver* aObserver)
{
    NS_PRECONDITION(aObserver != nsnull, "null ptr");
    if (! aObserver)
        return NS_ERROR_NULL_POINTER;

    mObservers.AppendObject(aObserver);
    mNumObservers = mObservers.Count();

    return NS_OK;
}

NS_IMETHODIMP
InMemoryDataSource::RemoveObserver(nsIRDFObserver* aObserver)
{
    NS_PRECONDITION(aObserver != nsnull, "null ptr");
    if (! aObserver)
        return NS_ERROR_NULL_POINTER;

    mObservers.RemoveObject(aObserver);
    
    
    mNumObservers = mObservers.Count();

    return NS_OK;
}

NS_IMETHODIMP 
InMemoryDataSource::HasArcIn(nsIRDFNode *aNode, nsIRDFResource *aArc, PRBool *result)
{
    Assertion* ass = GetReverseArcs(aNode);
    while (ass) {
        nsIRDFResource* elbow = ass->u.as.mProperty;
        if (elbow == aArc) {
            *result = PR_TRUE;
            return NS_OK;
        }
        ass = ass->u.as.mInvNext;
    }
    *result = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP 
InMemoryDataSource::HasArcOut(nsIRDFResource *aSource, nsIRDFResource *aArc, PRBool *result)
{
    Assertion* ass = GetForwardArcs(aSource);
    if (ass && ass->mHashEntry) {
        PLDHashEntryHdr* hdr = PL_DHashTableOperate(ass->u.hash.mPropertyHash,
            aArc, PL_DHASH_LOOKUP);
        Assertion* val = PL_DHASH_ENTRY_IS_BUSY(hdr)
            ? NS_REINTERPRET_CAST(Entry*, hdr)->mAssertions
            : nsnull;
        if (val) {
            *result = PR_TRUE;
            return NS_OK;
        }
        ass = ass->mNext;
    }
    while (ass) {
        nsIRDFResource* elbow = ass->u.as.mProperty;
        if (elbow == aArc) {
            *result = PR_TRUE;
            return NS_OK;
        }
        ass = ass->mNext;
    }
    *result = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
InMemoryDataSource::ArcLabelsIn(nsIRDFNode* aTarget, nsISimpleEnumerator** aResult)
{
    NS_PRECONDITION(aTarget != nsnull, "null ptr");
    if (! aTarget)
        return NS_ERROR_NULL_POINTER;

    InMemoryArcsEnumeratorImpl* result =
        InMemoryArcsEnumeratorImpl::Create(this, nsnull, aTarget);

    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(result);
    *aResult = result;

    return NS_OK;
}

NS_IMETHODIMP
InMemoryDataSource::ArcLabelsOut(nsIRDFResource* aSource, nsISimpleEnumerator** aResult)
{
    NS_PRECONDITION(aSource != nsnull, "null ptr");
    if (! aSource)
        return NS_ERROR_NULL_POINTER;

    InMemoryArcsEnumeratorImpl* result =
        InMemoryArcsEnumeratorImpl::Create(this, aSource, nsnull);

    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(result);
    *aResult = result;

    return NS_OK;
}

PLDHashOperator PR_CALLBACK
InMemoryDataSource::ResourceEnumerator(PLDHashTable* aTable,
                                       PLDHashEntryHdr* aHdr,
                                       PRUint32 aNumber, void* aArg)
{
    Entry* entry = NS_REINTERPRET_CAST(Entry*, aHdr);
    nsISupportsArray* resources = NS_STATIC_CAST(nsISupportsArray*, aArg);

    resources->AppendElement(entry->mNode);
    return PL_DHASH_NEXT;
}


NS_IMETHODIMP
InMemoryDataSource::GetAllResources(nsISimpleEnumerator** aResult)
{
    nsresult rv;

    nsCOMPtr<nsISupportsArray> values;
    rv = NS_NewISupportsArray(getter_AddRefs(values));
    if (NS_FAILED(rv)) return rv;

    
    PL_DHashTableEnumerate(&mForwardArcs, ResourceEnumerator, values.get());

    return NS_NewArrayEnumerator(aResult, values);
}

NS_IMETHODIMP
InMemoryDataSource::GetAllCmds(nsIRDFResource* source,
                               nsISimpleEnumerator** commands)
{
    return(NS_NewEmptyEnumerator(commands));
}

NS_IMETHODIMP
InMemoryDataSource::IsCommandEnabled(nsISupportsArray* aSources,
                                     nsIRDFResource*   aCommand,
                                     nsISupportsArray* aArguments,
                                     PRBool* aResult)
{
    *aResult = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
InMemoryDataSource::DoCommand(nsISupportsArray* aSources,
                              nsIRDFResource*   aCommand,
                              nsISupportsArray* aArguments)
{
    return NS_OK;
}

NS_IMETHODIMP
InMemoryDataSource::BeginUpdateBatch()
{
    for (PRInt32 i = PRInt32(mNumObservers) - 1; mPropagateChanges && i >= 0; --i) {
        nsIRDFObserver* obs = mObservers[i];
        obs->OnBeginUpdateBatch(this);
    }
    return NS_OK;
}

NS_IMETHODIMP
InMemoryDataSource::EndUpdateBatch()
{
    for (PRInt32 i = PRInt32(mNumObservers) - 1; mPropagateChanges && i >= 0; --i) {
        nsIRDFObserver* obs = mObservers[i];
        obs->OnEndUpdateBatch(this);
    }
    return NS_OK;
}






NS_IMETHODIMP
InMemoryDataSource::EnsureFastContainment(nsIRDFResource* aSource)
{
    Assertion *as = GetForwardArcs(aSource);
    PRBool  haveHash = (as) ? as->mHashEntry : PR_FALSE;
    
    
    if (haveHash)   return(NS_OK);

    
    Assertion *hashAssertion = Assertion::Create(mAllocator, aSource);
    NS_ASSERTION(hashAssertion, "unable to Assertion::Create");
    if (!hashAssertion) return(NS_ERROR_OUT_OF_MEMORY);

    
    hashAssertion->AddRef();

    register Assertion *first = GetForwardArcs(aSource);
    SetForwardArcs(aSource, hashAssertion);

    
    PLDHashTable *table = hashAssertion->u.hash.mPropertyHash;
    Assertion *nextRef;
    while(first) {
        nextRef = first->mNext;
        nsIRDFResource *prop = first->u.as.mProperty;

        PLDHashEntryHdr* hdr = PL_DHashTableOperate(table,
            prop, PL_DHASH_LOOKUP);
        Assertion* val = PL_DHASH_ENTRY_IS_BUSY(hdr)
            ? NS_REINTERPRET_CAST(Entry*, hdr)->mAssertions
            : nsnull;
        if (val) {
            first->mNext = val->mNext;
            val->mNext = first;
        }
        else {
            PLDHashEntryHdr* hdr = PL_DHashTableOperate(table,
                                            prop, PL_DHASH_ADD);
            if (hdr) {
                Entry* entry = NS_REINTERPRET_CAST(Entry*, hdr);
                entry->mNode = prop;
                entry->mAssertions = first;
                first->mNext = nsnull;
            }
        }
        first = nextRef;
    }
    return(NS_OK);
}




NS_IMETHODIMP
InMemoryDataSource::GetPropagateChanges(PRBool* aPropagateChanges)
{
    *aPropagateChanges = mPropagateChanges;
    return NS_OK;
}

NS_IMETHODIMP
InMemoryDataSource::SetPropagateChanges(PRBool aPropagateChanges)
{
    mPropagateChanges = aPropagateChanges;
    return NS_OK;
}





NS_IMETHODIMP
InMemoryDataSource::Mark(nsIRDFResource* aSource,
                         nsIRDFResource* aProperty,
                         nsIRDFNode* aTarget,
                         PRBool aTruthValue,
                         PRBool* aDidMark)
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

    Assertion *as = GetForwardArcs(aSource);
    if (as && as->mHashEntry) {
        PLDHashEntryHdr* hdr = PL_DHashTableOperate(as->u.hash.mPropertyHash,
            aProperty, PL_DHASH_LOOKUP);
        Assertion* val = PL_DHASH_ENTRY_IS_BUSY(hdr)
            ? NS_REINTERPRET_CAST(Entry*, hdr)->mAssertions
            : nsnull;
        while (val) {
            if ((val->u.as.mTarget == aTarget) &&
                (aTruthValue == (val->u.as.mTruthValue))) {

                
                as->Mark();
                *aDidMark = PR_TRUE;

#ifdef PR_LOGGING
                LogOperation("MARK", aSource, aProperty, aTarget, aTruthValue);
#endif

                return NS_OK;
            }
            val = val->mNext;
        }
    }
    else for (; as != nsnull; as = as->mNext) {
        
        if (aTarget != as->u.as.mTarget)
            continue;

        if (aProperty != as->u.as.mProperty)
            continue;

        if (aTruthValue != (as->u.as.mTruthValue))
            continue;

        
        as->Mark();
        *aDidMark = PR_TRUE;

#ifdef PR_LOGGING
        LogOperation("MARK", aSource, aProperty, aTarget, aTruthValue);
#endif

        return NS_OK;
    }

    
    *aDidMark = PR_FALSE;
    return NS_OK;
}


struct SweepInfo {
    Assertion* mUnassertList;
    PLDHashTable* mReverseArcs;
    nsFixedSizeAllocator* mAllocator;
};

NS_IMETHODIMP
InMemoryDataSource::Sweep()
{
    SweepInfo info = { nsnull, &mReverseArcs, &mAllocator};

    
    PL_DHashTableEnumerate(&mForwardArcs, SweepForwardArcsEntries, &info);

    
    Assertion* as = info.mUnassertList;
    while (as) {
#ifdef PR_LOGGING
        LogOperation("SWEEP", as->mSource, as->u.as.mProperty, as->u.as.mTarget, as->u.as.mTruthValue);
#endif
        if (!(as->mHashEntry))
        {
            for (PRInt32 i = PRInt32(mNumObservers) - 1; mPropagateChanges && i >= 0; --i) {
                nsIRDFObserver* obs = mObservers[i];
                
                obs->OnUnassert(this, as->mSource, as->u.as.mProperty, as->u.as.mTarget);
                
            }
        }

        Assertion* doomed = as;
        as = as->mNext;

        
        doomed->mNext = doomed->u.as.mInvNext = nsnull;
        doomed->Release(mAllocator);
    }

    return NS_OK;
}


PLDHashOperator PR_CALLBACK
InMemoryDataSource::SweepForwardArcsEntries(PLDHashTable* aTable,
                                            PLDHashEntryHdr* aHdr,
                                            PRUint32 aNumber, void* aArg)
{
    PLDHashOperator result = PL_DHASH_NEXT;
    Entry* entry = NS_REINTERPRET_CAST(Entry*, aHdr);
    SweepInfo* info = NS_STATIC_CAST(SweepInfo*, aArg);

    Assertion* as = entry->mAssertions;
    if (as && (as->mHashEntry))
    {
        
        PL_DHashTableEnumerate(as->u.hash.mPropertyHash, 
                               SweepForwardArcsEntries, info);

        
        if (!as->u.hash.mPropertyHash->entryCount) {
            Assertion::Destroy(*info->mAllocator, as);
            result = PL_DHASH_REMOVE;
        }

        return result;
    }

    Assertion* prev = nsnull;
    while (as) {
        if (as->IsMarked()) {
            prev = as;
            as->Unmark();
            as = as->mNext;
        }
        else {
            
            Assertion* next = as->mNext;
            if (prev) {
                prev->mNext = next;
            }
            else {
                
                entry->mAssertions = next;
            }

            
            PLDHashEntryHdr* hdr =
                PL_DHashTableOperate(info->mReverseArcs, as->u.as.mTarget, PL_DHASH_LOOKUP);
            NS_ASSERTION(PL_DHASH_ENTRY_IS_BUSY(hdr), "no assertion in reverse arcs");

            Entry* rentry = NS_REINTERPRET_CAST(Entry*, hdr);
            Assertion* ras = rentry->mAssertions;
            Assertion* rprev = nsnull;
            while (ras) {
                if (ras == as) {
                    if (rprev) {
                        rprev->u.as.mInvNext = ras->u.as.mInvNext;
                    }
                    else {
                        
                        rentry->mAssertions = ras->u.as.mInvNext;
                    }
                    as->u.as.mInvNext = nsnull; 
                    break;
                }
                rprev = ras;
                ras = ras->u.as.mInvNext;
            }

            
            if (! rentry->mAssertions)
            {
                PL_DHashTableRawRemove(info->mReverseArcs, hdr);
            }

            
            as->mNext = info->mUnassertList;
            info->mUnassertList = as;

            
            as = next;
        }
    }

    
    if (! entry->mAssertions)
        result = PL_DHASH_REMOVE;

    return result;
}




class VisitorClosure
{
public:
    VisitorClosure(rdfITripleVisitor* aVisitor) :
        mVisitor(aVisitor),
        mRv(NS_OK)
    {}
    rdfITripleVisitor* mVisitor;
    nsresult mRv;
};

PLDHashOperator PR_CALLBACK
SubjectEnumerator(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
                  PRUint32 aNumber, void* aArg) {
    Entry* entry = NS_REINTERPRET_CAST(Entry*, aHdr);
    VisitorClosure* closure = NS_STATIC_CAST(VisitorClosure*, aArg);

    nsresult rv;
    nsCOMPtr<nsIRDFNode> subject = do_QueryInterface(entry->mNode, &rv);
    NS_ENSURE_SUCCESS(rv, PL_DHASH_NEXT);

    closure->mRv = closure->mVisitor->Visit(subject, nsnull, nsnull, PR_TRUE);
    if (NS_FAILED(closure->mRv) || closure->mRv == NS_RDF_STOP_VISIT)
        return PL_DHASH_STOP;

    return PL_DHASH_NEXT;
}

NS_IMETHODIMP
InMemoryDataSource::VisitAllSubjects(rdfITripleVisitor *aVisitor)
{
    
    ++mReadCount;

    
    VisitorClosure cls(aVisitor);
    PL_DHashTableEnumerate(&mForwardArcs, SubjectEnumerator, &cls);

    
    --mReadCount;

    return cls.mRv;
} 

class TriplesInnerClosure
{
public:
    TriplesInnerClosure(nsIRDFNode* aSubject, VisitorClosure* aClosure) :
        mSubject(aSubject), mOuter(aClosure) {}
    nsIRDFNode* mSubject;
    VisitorClosure* mOuter;
};

PLDHashOperator PR_CALLBACK
TriplesInnerEnumerator(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
                  PRUint32 aNumber, void* aArg) {
    Entry* entry = NS_REINTERPRET_CAST(Entry*, aHdr);
    Assertion* assertion = entry->mAssertions;
    TriplesInnerClosure* closure = 
        NS_STATIC_CAST(TriplesInnerClosure*, aArg);
    while (assertion) {
        NS_ASSERTION(!assertion->mHashEntry, "shouldn't have to hashes");
        VisitorClosure* cls = closure->mOuter;
        cls->mRv = cls->mVisitor->Visit(closure->mSubject,
                                        assertion->u.as.mProperty,
                                        assertion->u.as.mTarget,
                                        assertion->u.as.mTruthValue);
        if (NS_FAILED(cls->mRv) || cls->mRv == NS_RDF_STOP_VISIT) {
            return PL_DHASH_STOP;
        }
        assertion = assertion->mNext;
    }
    return PL_DHASH_NEXT;
}
PLDHashOperator PR_CALLBACK
TriplesEnumerator(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
                  PRUint32 aNumber, void* aArg) {
    Entry* entry = NS_REINTERPRET_CAST(Entry*, aHdr);
    VisitorClosure* closure = NS_STATIC_CAST(VisitorClosure*, aArg);

    nsresult rv;
    nsCOMPtr<nsIRDFNode> subject = do_QueryInterface(entry->mNode, &rv);
    NS_ENSURE_SUCCESS(rv, PL_DHASH_NEXT);

    if (entry->mAssertions->mHashEntry) {
        TriplesInnerClosure cls(subject, closure);
        PL_DHashTableEnumerate(entry->mAssertions->u.hash.mPropertyHash,
                               TriplesInnerEnumerator, &cls);
        if (NS_FAILED(closure->mRv)) {
            return PL_DHASH_STOP;
        }
        return PL_DHASH_NEXT;
    }
    Assertion* assertion = entry->mAssertions;
    while (assertion) {
        NS_ASSERTION(!assertion->mHashEntry, "shouldn't have to hashes");
        closure->mRv = closure->mVisitor->Visit(subject,
                                                assertion->u.as.mProperty,
                                                assertion->u.as.mTarget,
                                                assertion->u.as.mTruthValue);
        if (NS_FAILED(closure->mRv) || closure->mRv == NS_RDF_STOP_VISIT) {
            return PL_DHASH_STOP;
        }
        assertion = assertion->mNext;
    }
    return PL_DHASH_NEXT;
}
NS_IMETHODIMP
InMemoryDataSource::VisitAllTriples(rdfITripleVisitor *aVisitor)
{
    
    ++mReadCount;

    
    VisitorClosure cls(aVisitor);
    PL_DHashTableEnumerate(&mForwardArcs, TriplesEnumerator, &cls);

    
    --mReadCount;

    return cls.mRv;
} 



