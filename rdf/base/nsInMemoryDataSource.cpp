





































#include "nsAgg.h"
#include "nsCOMPtr.h"
#include "nscore.h"
#include "nsArrayEnumerator.h"
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
#include "nsTArray.h"
#include "nsCRT.h"
#include "nsRDFCID.h"
#include "nsRDFBaseDataSources.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsXPIDLString.h"
#include "rdfutil.h"
#include "pldhash.h"
#include "plstr.h"
#include "prlog.h"
#include "rdf.h"

#include "rdfIDataSource.h"
#include "rdfITripleVisitor.h"








class Assertion
{
public:
    static PLDHashOperator
    DeletePropertyHashEntry(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
                           uint32_t aNumber, void* aArg);

    Assertion(nsIRDFResource* aSource,      
              nsIRDFResource* aProperty,
              nsIRDFNode* aTarget,
              bool aTruthValue);
    explicit Assertion(nsIRDFResource* aSource);     

private:
    ~Assertion();

public:
    void AddRef() {
        if (mRefCnt == UINT16_MAX) {
            NS_WARNING("refcount overflow, leaking Assertion");
            return;
        }
        ++mRefCnt;
    }

    void Release() {
        if (mRefCnt == UINT16_MAX) {
            NS_WARNING("refcount overflow, leaking Assertion");
            return;
        }
        if (--mRefCnt == 0)
            delete this;
    }

    
    inline  void    Mark()      { u.as.mMarked = true; }
    inline  bool    IsMarked()  { return u.as.mMarked; }
    inline  void    Unmark()    { u.as.mMarked = false; }

    

    
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
            
            bool            mTruthValue;
            bool            mMarked;
        } as;
    } u;

    
    
    
    uint16_t                    mRefCnt;
    bool                        mHashEntry;
};


struct Entry : PLDHashEntryHdr {
    nsIRDFNode*     mNode;
    Assertion*      mAssertions;
};


Assertion::Assertion(nsIRDFResource* aSource)
    : mSource(aSource),
      mNext(nullptr),
      mRefCnt(0),
      mHashEntry(true)
{
    MOZ_COUNT_CTOR(Assertion);

    NS_ADDREF(mSource);

    u.hash.mPropertyHash =
        PL_NewDHashTable(PL_DHashGetStubOps(), sizeof(Entry));
}

Assertion::Assertion(nsIRDFResource* aSource,
                     nsIRDFResource* aProperty,
                     nsIRDFNode* aTarget,
                     bool aTruthValue)
    : mSource(aSource),
      mNext(nullptr),
      mRefCnt(0),
      mHashEntry(false)
{
    MOZ_COUNT_CTOR(Assertion);

    u.as.mProperty = aProperty;
    u.as.mTarget = aTarget;

    NS_ADDREF(mSource);
    NS_ADDREF(u.as.mProperty);
    NS_ADDREF(u.as.mTarget);

    u.as.mInvNext = nullptr;
    u.as.mTruthValue = aTruthValue;
    u.as.mMarked = false;
}

Assertion::~Assertion()
{
    if (mHashEntry && u.hash.mPropertyHash) {
        PL_DHashTableEnumerate(u.hash.mPropertyHash, DeletePropertyHashEntry,
                               nullptr);
        PL_DHashTableDestroy(u.hash.mPropertyHash);
        u.hash.mPropertyHash = nullptr;
    }

    MOZ_COUNT_DTOR(Assertion);
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

PLDHashOperator
Assertion::DeletePropertyHashEntry(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
                                           uint32_t aNumber, void* aArg)
{
    Entry* entry = static_cast<Entry*>(aHdr);

    Assertion* as = entry->mAssertions;
    while (as) {
        Assertion* doomed = as;
        as = as->mNext;

        
        doomed->mNext = doomed->u.as.mInvNext = nullptr;
        doomed->Release();
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
    
    
    
    
    
    PLDHashTable mForwardArcs; 
    PLDHashTable mReverseArcs; 

    nsCOMArray<nsIRDFObserver> mObservers;  
    uint32_t                   mNumObservers;

    
    
    uint32_t mReadCount;

    static PLDHashOperator
    DeleteForwardArcsEntry(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
                           uint32_t aNumber, void* aArg);

    static PLDHashOperator
    ResourceEnumerator(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
                       uint32_t aNumber, void* aArg);

    friend class InMemoryArcsEnumeratorImpl;
    friend class InMemoryAssertionEnumeratorImpl;
    friend class InMemoryResourceEnumeratorImpl; 

    
    nsresult
    LockedAssert(nsIRDFResource* source, 
                 nsIRDFResource* property, 
                 nsIRDFNode* target,
                 bool tv);

    nsresult
    LockedUnassert(nsIRDFResource* source,
                   nsIRDFResource* property,
                   nsIRDFNode* target);

    explicit InMemoryDataSource(nsISupports* aOuter);
    virtual ~InMemoryDataSource();
    nsresult Init();

    friend nsresult
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
    static PLDHashOperator
    SweepForwardArcsEntries(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
                            uint32_t aNumber, void* aArg);

public:
    
    Assertion*
    GetForwardArcs(nsIRDFResource* u) {
        PLDHashEntryHdr* hdr = PL_DHashTableSearch(&mForwardArcs, u);
        return hdr ? static_cast<Entry*>(hdr)->mAssertions : nullptr;
    }

    Assertion*
    GetReverseArcs(nsIRDFNode* v) {
        PLDHashEntryHdr* hdr = PL_DHashTableSearch(&mReverseArcs, v);
        return hdr ? static_cast<Entry*>(hdr)->mAssertions : nullptr;
    }

    void
    SetForwardArcs(nsIRDFResource* u, Assertion* as) {
        if (as) {
            Entry* entry = static_cast<Entry*>
                (PL_DHashTableAdd(&mForwardArcs, u, mozilla::fallible));
            if (entry) {
                entry->mNode = u;
                entry->mAssertions = as;
            }
        }
        else {
            PL_DHashTableRemove(&mForwardArcs, u);
        }
    }

    void
    SetReverseArcs(nsIRDFNode* v, Assertion* as) {
        if (as) {
            Entry* entry = static_cast<Entry*>
                (PL_DHashTableAdd(&mReverseArcs, v, mozilla::fallible));
            if (entry) {
                entry->mNode = v;
                entry->mAssertions = as;
            }
        }
        else {
            PL_DHashTableRemove(&mReverseArcs, v);
        }
    }

#ifdef PR_LOGGING
    void
    LogOperation(const char* aOperation,
                 nsIRDFResource* asource,
                 nsIRDFResource* aProperty,
                 nsIRDFNode* aTarget,
                 bool aTruthValue = true);
#endif

    bool    mPropagateChanges;

private:
#ifdef PR_LOGGING
    static PRLogModuleInfo* gLog;
#endif
};

#ifdef PR_LOGGING
PRLogModuleInfo* InMemoryDataSource::gLog;
#endif









class InMemoryAssertionEnumeratorImpl : public nsISimpleEnumerator
{
private:
    InMemoryDataSource* mDataSource;
    nsIRDFResource* mSource;
    nsIRDFResource* mProperty;
    nsIRDFNode*     mTarget;
    nsIRDFNode*     mValue;
    bool            mTruthValue;
    Assertion*      mNextAssertion;
    nsCOMPtr<nsISupportsArray> mHashArcs;

    virtual ~InMemoryAssertionEnumeratorImpl();

public:
    InMemoryAssertionEnumeratorImpl(InMemoryDataSource* aDataSource,
                                    nsIRDFResource* aSource,
                                    nsIRDFResource* aProperty,
                                    nsIRDFNode* aTarget,
                                    bool aTruthValue);

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSISIMPLEENUMERATOR
};




InMemoryAssertionEnumeratorImpl::InMemoryAssertionEnumeratorImpl(
                 InMemoryDataSource* aDataSource,
                 nsIRDFResource* aSource,
                 nsIRDFResource* aProperty,
                 nsIRDFNode* aTarget,
                 bool aTruthValue)
    : mDataSource(aDataSource),
      mSource(aSource),
      mProperty(aProperty),
      mTarget(aTarget),
      mValue(nullptr),
      mTruthValue(aTruthValue),
      mNextAssertion(nullptr)
{
    NS_ADDREF(mDataSource);
    NS_IF_ADDREF(mSource);
    NS_ADDREF(mProperty);
    NS_IF_ADDREF(mTarget);

    if (mSource) {
        mNextAssertion = mDataSource->GetForwardArcs(mSource);

        if (mNextAssertion && mNextAssertion->mHashEntry) {
            
            PLDHashEntryHdr* hdr =
                PL_DHashTableSearch(mNextAssertion->u.hash.mPropertyHash,
                                    aProperty);
            mNextAssertion =
                hdr ? static_cast<Entry*>(hdr)->mAssertions : nullptr;
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
        mNextAssertion->Release();

    NS_IF_RELEASE(mDataSource);
    NS_IF_RELEASE(mSource);
    NS_IF_RELEASE(mProperty);
    NS_IF_RELEASE(mTarget);
    NS_IF_RELEASE(mValue);
}

NS_IMPL_ADDREF(InMemoryAssertionEnumeratorImpl)
NS_IMPL_RELEASE(InMemoryAssertionEnumeratorImpl)
NS_IMPL_QUERY_INTERFACE(InMemoryAssertionEnumeratorImpl, nsISimpleEnumerator)

NS_IMETHODIMP
InMemoryAssertionEnumeratorImpl::HasMoreElements(bool* aResult)
{
    if (mValue) {
        *aResult = true;
        return NS_OK;
    }

    while (mNextAssertion) {
        bool foundIt = false;
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
            foundIt = true;
        }

        
        Assertion* as = mNextAssertion;

        
        mNextAssertion = (mSource) ? mNextAssertion->mNext : mNextAssertion->u.as.mInvNext;

        
        if (mNextAssertion)
            mNextAssertion->AddRef();

        
        as->Release();

        if (foundIt) {
            *aResult = true;
            return NS_OK;
        }
    }

    *aResult = false;
    return NS_OK;
}


NS_IMETHODIMP
InMemoryAssertionEnumeratorImpl::GetNext(nsISupports** aResult)
{
    nsresult rv;

    bool hasMore;
    rv = HasMoreElements(&hasMore);
    if (NS_FAILED(rv)) return rv;

    if (! hasMore)
        return NS_ERROR_UNEXPECTED;

    
    *aResult = mValue;
    mValue = nullptr;

    return NS_OK;
}












class InMemoryArcsEnumeratorImpl : public nsISimpleEnumerator
{
private:
    InMemoryDataSource* mDataSource;
    nsIRDFResource*     mSource;
    nsIRDFNode*         mTarget;
    nsAutoTArray<nsCOMPtr<nsIRDFResource>, 8> mAlreadyReturned;
    nsIRDFResource*     mCurrent;
    Assertion*          mAssertion;
    nsCOMPtr<nsISupportsArray> mHashArcs;

    static PLDHashOperator
    ArcEnumerator(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
                       uint32_t aNumber, void* aArg);

    virtual ~InMemoryArcsEnumeratorImpl();

public:
    InMemoryArcsEnumeratorImpl(InMemoryDataSource* aDataSource,
                               nsIRDFResource* aSource,
                               nsIRDFNode* aTarget);

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSISIMPLEENUMERATOR
};


PLDHashOperator
InMemoryArcsEnumeratorImpl::ArcEnumerator(PLDHashTable* aTable,
                                       PLDHashEntryHdr* aHdr,
                                       uint32_t aNumber, void* aArg)
{
    Entry* entry = static_cast<Entry*>(aHdr);
    nsISupportsArray* resources = static_cast<nsISupportsArray*>(aArg);

    resources->AppendElement(entry->mNode);
    return PL_DHASH_NEXT;
}


InMemoryArcsEnumeratorImpl::InMemoryArcsEnumeratorImpl(InMemoryDataSource* aDataSource,
                                                       nsIRDFResource* aSource,
                                                       nsIRDFNode* aTarget)
    : mDataSource(aDataSource),
      mSource(aSource),
      mTarget(aTarget),
      mCurrent(nullptr)
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
            mAssertion = nullptr;
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
}

NS_IMPL_ADDREF(InMemoryArcsEnumeratorImpl)
NS_IMPL_RELEASE(InMemoryArcsEnumeratorImpl)
NS_IMPL_QUERY_INTERFACE(InMemoryArcsEnumeratorImpl, nsISimpleEnumerator)

NS_IMETHODIMP
InMemoryArcsEnumeratorImpl::HasMoreElements(bool* aResult)
{
    NS_PRECONDITION(aResult != nullptr, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    if (mCurrent) {
        *aResult = true;
        return NS_OK;
    }

    if (mHashArcs) {
        uint32_t    itemCount;
        nsresult    rv;
        if (NS_FAILED(rv = mHashArcs->Count(&itemCount)))   return(rv);
        if (itemCount > 0) {
            --itemCount;
            nsCOMPtr<nsIRDFResource> tmp = do_QueryElementAt(mHashArcs, itemCount);
            tmp.forget(&mCurrent);
            mHashArcs->RemoveElementAt(itemCount);
            *aResult = true;
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

            bool alreadyReturned = false;
            for (int32_t i = mAlreadyReturned.Length() - 1; i >= 0; --i) {
                if (mAlreadyReturned[i] == next) {
                    alreadyReturned = true;
                    break;
                }
            }

            if (! alreadyReturned) {
                mCurrent = next;
                NS_ADDREF(mCurrent);
                *aResult = true;
                return NS_OK;
            }
        }

    *aResult = false;
    return NS_OK;
}


NS_IMETHODIMP
InMemoryArcsEnumeratorImpl::GetNext(nsISupports** aResult)
{
    nsresult rv;

    bool hasMore;
    rv = HasMoreElements(&hasMore);
    if (NS_FAILED(rv)) return rv;

    if (! hasMore)
        return NS_ERROR_UNEXPECTED;

    
    
    mAlreadyReturned.AppendElement(mCurrent);

    
    *aResult = mCurrent;
    mCurrent = nullptr;

    return NS_OK;
}





nsresult
NS_NewRDFInMemoryDataSource(nsISupports* aOuter, const nsIID& aIID, void** aResult)
{
    NS_PRECONDITION(aResult != nullptr, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;
    *aResult = nullptr;

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

    mPropagateChanges = true;
    MOZ_COUNT_CTOR(InMemoryDataSource);
}


nsresult
InMemoryDataSource::Init()
{
    PL_DHashTableInit(&mForwardArcs, PL_DHashGetStubOps(), sizeof(Entry));
    PL_DHashTableInit(&mReverseArcs, PL_DHashGetStubOps(), sizeof(Entry));

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

    if (mForwardArcs.IsInitialized()) {
        
        
        
        
        PL_DHashTableEnumerate(&mForwardArcs, DeleteForwardArcsEntry, nullptr);
        PL_DHashTableFinish(&mForwardArcs);
    }
    if (mReverseArcs.IsInitialized())
        PL_DHashTableFinish(&mReverseArcs);

    PR_LOG(gLog, PR_LOG_NOTICE,
           ("InMemoryDataSource(%p): destroyed.", this));

    MOZ_COUNT_DTOR(InMemoryDataSource);
}

PLDHashOperator
InMemoryDataSource::DeleteForwardArcsEntry(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
                                           uint32_t aNumber, void* aArg)
{
    Entry* entry = static_cast<Entry*>(aHdr);

    Assertion* as = entry->mAssertions;
    while (as) {
        Assertion* doomed = as;
        as = as->mNext;

        
        doomed->mNext = doomed->u.as.mInvNext = nullptr;
        doomed->Release();
    }
    return PL_DHASH_NEXT;
}




NS_IMPL_CYCLE_COLLECTION_CLASS(InMemoryDataSource)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(InMemoryDataSource)
    NS_IMPL_CYCLE_COLLECTION_UNLINK(mObservers)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_AGGREGATED(InMemoryDataSource)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mObservers)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_AGGREGATED(InMemoryDataSource)
NS_INTERFACE_MAP_BEGIN_AGGREGATED(InMemoryDataSource)
    NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION_AGGREGATED(InMemoryDataSource)
    NS_INTERFACE_MAP_ENTRY(nsIRDFDataSource)
    NS_INTERFACE_MAP_ENTRY(nsIRDFInMemoryDataSource)
    NS_INTERFACE_MAP_ENTRY(nsIRDFPropagatableDataSource)
    NS_INTERFACE_MAP_ENTRY(nsIRDFPurgeableDataSource)
    NS_INTERFACE_MAP_ENTRY(rdfIDataSource)
NS_INTERFACE_MAP_END




#ifdef PR_LOGGING
void
InMemoryDataSource::LogOperation(const char* aOperation,
                                 nsIRDFResource* aSource,
                                 nsIRDFResource* aProperty,
                                 nsIRDFNode* aTarget,
                                 bool aTruthValue)
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

    if ((resource = do_QueryInterface(aTarget)) != nullptr) {
        resource->GetValue(getter_Copies(uri));
        PR_LogPrint
           ("  -->[(%p)%s]", aTarget, (const char*) uri);
    }
    else if ((literal = do_QueryInterface(aTarget)) != nullptr) {
        nsXPIDLString value;
        literal->GetValue(getter_Copies(value));
        nsAutoString valueStr(value);
        char* valueCStr = ToNewCString(valueStr);

        PR_LogPrint
           ("  -->(\"%s\")\n", valueCStr);

        free(valueCStr);
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
    NS_PRECONDITION(uri != nullptr, "null ptr");
    if (! uri)
        return NS_ERROR_NULL_POINTER;

    *uri = nullptr;
    return NS_OK;
}

NS_IMETHODIMP
InMemoryDataSource::GetSource(nsIRDFResource* property,
                              nsIRDFNode* target,
                              bool tv,
                              nsIRDFResource** source)
{
    NS_PRECONDITION(source != nullptr, "null ptr");
    if (! source)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(property != nullptr, "null ptr");
    if (! property)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(target != nullptr, "null ptr");
    if (! target)
        return NS_ERROR_NULL_POINTER;

    for (Assertion* as = GetReverseArcs(target); as; as = as->u.as.mInvNext) {
        if ((property == as->u.as.mProperty) && (tv == as->u.as.mTruthValue)) {
            *source = as->mSource;
            NS_ADDREF(*source);
            return NS_OK;
        }
    }
    *source = nullptr;
    return NS_RDF_NO_VALUE;
}

NS_IMETHODIMP
InMemoryDataSource::GetTarget(nsIRDFResource* source,
                              nsIRDFResource* property,
                              bool tv,
                              nsIRDFNode** target)
{
    NS_PRECONDITION(source != nullptr, "null ptr");
    if (! source)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(property != nullptr, "null ptr");
    if (! property)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(target != nullptr, "null ptr");
    if (! target)
        return NS_ERROR_NULL_POINTER;

    Assertion *as = GetForwardArcs(source);
    if (as && as->mHashEntry) {
        PLDHashEntryHdr* hdr =
            PL_DHashTableSearch(as->u.hash.mPropertyHash, property);
        Assertion* val = hdr ? static_cast<Entry*>(hdr)->mAssertions : nullptr;
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
    for (; as != nullptr; as = as->mNext) {
        if ((property == as->u.as.mProperty) && (tv == (as->u.as.mTruthValue))) {
            *target = as->u.as.mTarget;
            NS_ADDREF(*target);
            return NS_OK;
        }
    }

    
    
    *target = nullptr;
    return NS_RDF_NO_VALUE;
}

NS_IMETHODIMP
InMemoryDataSource::HasAssertion(nsIRDFResource* source,
                                 nsIRDFResource* property,
                                 nsIRDFNode* target,
                                 bool tv,
                                 bool* hasAssertion)
{
    if (! source)
        return NS_ERROR_NULL_POINTER;

    if (! property)
        return NS_ERROR_NULL_POINTER;

    if (! target)
        return NS_ERROR_NULL_POINTER;

    Assertion *as = GetForwardArcs(source);
    if (as && as->mHashEntry) {
        PLDHashEntryHdr* hdr =
            PL_DHashTableSearch(as->u.hash.mPropertyHash, property);
        Assertion* val = hdr ? static_cast<Entry*>(hdr)->mAssertions : nullptr;
        while (val) {
            if ((val->u.as.mTarget == target) && (tv == (val->u.as.mTruthValue))) {
                *hasAssertion = true;
                return NS_OK;
            }
            val = val->mNext;
        }
    }
    else
    for (; as != nullptr; as = as->mNext) {
        
        if (target != as->u.as.mTarget)
            continue;

        if (property != as->u.as.mProperty)
            continue;

        if (tv != (as->u.as.mTruthValue))
            continue;

        
        *hasAssertion = true;
        return NS_OK;
    }

    
    *hasAssertion = false;
    return NS_OK;
}

NS_IMETHODIMP
InMemoryDataSource::GetSources(nsIRDFResource* aProperty,
                               nsIRDFNode* aTarget,
                               bool aTruthValue,
                               nsISimpleEnumerator** aResult)
{
    NS_PRECONDITION(aProperty != nullptr, "null ptr");
    if (! aProperty)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aTarget != nullptr, "null ptr");
    if (! aTarget)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aResult != nullptr, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    InMemoryAssertionEnumeratorImpl* result =
        new InMemoryAssertionEnumeratorImpl(this, nullptr, aProperty,
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
                               bool aTruthValue,
                               nsISimpleEnumerator** aResult)
{
    NS_PRECONDITION(aSource != nullptr, "null ptr");
    if (! aSource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aProperty != nullptr, "null ptr");
    if (! aProperty)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aResult != nullptr, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    InMemoryAssertionEnumeratorImpl* result =
        new InMemoryAssertionEnumeratorImpl(this, aSource, aProperty,
                                            nullptr, aTruthValue);

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
                                 bool aTruthValue)
{
#ifdef PR_LOGGING
    LogOperation("ASSERT", aSource, aProperty, aTarget, aTruthValue);
#endif

    Assertion* next = GetForwardArcs(aSource);
    Assertion* prev = next;
    Assertion* as = nullptr;

    bool    haveHash = (next) ? next->mHashEntry : false;
    if (haveHash) {
        PLDHashEntryHdr* hdr =
            PL_DHashTableSearch(next->u.hash.mPropertyHash, aProperty);
        Assertion* val = hdr ? static_cast<Entry*>(hdr)->mAssertions : nullptr;
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

    as = new Assertion(aSource, aProperty, aTarget, aTruthValue);
    if (! as)
        return NS_ERROR_OUT_OF_MEMORY;

    
    as->AddRef();

    if (haveHash)
    {
        PLDHashEntryHdr* hdr =
            PL_DHashTableSearch(next->u.hash.mPropertyHash, aProperty);
        Assertion *asRef =
            hdr ? static_cast<Entry*>(hdr)->mAssertions : nullptr;
        if (asRef)
        {
            as->mNext = asRef->mNext;
            asRef->mNext = as;
        }
        else
        {
            hdr = PL_DHashTableAdd(next->u.hash.mPropertyHash, aProperty,
                                   mozilla::fallible);
            if (hdr)
            {
                Entry* entry = static_cast<Entry*>(hdr);
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
                           bool aTruthValue) 
{
    NS_PRECONDITION(aSource != nullptr, "null ptr");
    if (! aSource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aProperty != nullptr, "null ptr");
    if (! aProperty)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aTarget != nullptr, "null ptr");
    if (! aTarget)
        return NS_ERROR_NULL_POINTER;

    if (mReadCount) {
        NS_WARNING("Writing to InMemoryDataSource during read\n");
        return NS_RDF_ASSERTION_REJECTED;
    }

    nsresult rv;
    rv = LockedAssert(aSource, aProperty, aTarget, aTruthValue);
    if (NS_FAILED(rv)) return rv;

    
    for (int32_t i = (int32_t)mNumObservers - 1; mPropagateChanges && i >= 0; --i) {
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
    Assertion* as = nullptr;

    bool    haveHash = (next) ? next->mHashEntry : false;
    if (haveHash) {
        PLDHashEntryHdr* hdr =
            PL_DHashTableSearch(next->u.hash.mPropertyHash, aProperty);
        prev = next = hdr ? static_cast<Entry*>(hdr)->mAssertions : nullptr;
        bool first = true;
        while (next) {
            if (aTarget == next->u.as.mTarget) {
                break;
            }
            first = false;
            prev = next;
            next = next->mNext;
        }
        
        if (!next)
            return NS_OK;

        as = next;

        if (first) {
            PL_DHashTableRawRemove(root->u.hash.mPropertyHash, hdr);

            if (next && next->mNext) {
                PLDHashEntryHdr* hdr =
                    PL_DHashTableAdd(root->u.hash.mPropertyHash, aProperty,
                                     mozilla::fallible);
                if (hdr) {
                    Entry* entry = static_cast<Entry*>(hdr);
                    entry->mNode = aProperty;
                    entry->mAssertions = next->mNext;
                }
            }
            else {
                
                if (!root->u.hash.mPropertyHash->EntryCount()) {
                    root->Release();
                    SetForwardArcs(aSource, nullptr);
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
    bool foundReverseArc = false;
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
            foundReverseArc = true;
#endif
            break;
        }
        prev = next;
        next = next->u.as.mInvNext;
    }

#ifdef DEBUG
    NS_ASSERTION(foundReverseArc, "in-memory db corrupted: unable to find reverse arc");
#endif

    
    as->mNext = as->u.as.mInvNext = nullptr;
    as->Release();

    return NS_OK;
}

NS_IMETHODIMP
InMemoryDataSource::Unassert(nsIRDFResource* aSource,
                             nsIRDFResource* aProperty,
                             nsIRDFNode* aTarget)
{
    NS_PRECONDITION(aSource != nullptr, "null ptr");
    if (! aSource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aProperty != nullptr, "null ptr");
    if (! aProperty)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aTarget != nullptr, "null ptr");
    if (! aTarget)
        return NS_ERROR_NULL_POINTER;

    if (mReadCount) {
        NS_WARNING("Writing to InMemoryDataSource during read\n");
        return NS_RDF_ASSERTION_REJECTED;
    }

    nsresult rv;

    rv = LockedUnassert(aSource, aProperty, aTarget);
    if (NS_FAILED(rv)) return rv;

    
    for (int32_t i = int32_t(mNumObservers) - 1; mPropagateChanges && i >= 0; --i) {
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
    NS_PRECONDITION(aSource != nullptr, "null ptr");
    if (! aSource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aProperty != nullptr, "null ptr");
    if (! aProperty)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aOldTarget != nullptr, "null ptr");
    if (! aOldTarget)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aNewTarget != nullptr, "null ptr");
    if (! aNewTarget)
        return NS_ERROR_NULL_POINTER;

    if (mReadCount) {
        NS_WARNING("Writing to InMemoryDataSource during read\n");
        return NS_RDF_ASSERTION_REJECTED;
    }

    nsresult rv;

    
    

    rv = LockedUnassert(aSource, aProperty, aOldTarget);
    if (NS_FAILED(rv)) return rv;

    rv = LockedAssert(aSource, aProperty, aNewTarget, true);
    if (NS_FAILED(rv)) return rv;

    
    for (int32_t i = int32_t(mNumObservers) - 1; mPropagateChanges && i >= 0; --i) {
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
    NS_PRECONDITION(aOldSource != nullptr, "null ptr");
    if (! aOldSource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aNewSource != nullptr, "null ptr");
    if (! aNewSource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aProperty != nullptr, "null ptr");
    if (! aProperty)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aTarget != nullptr, "null ptr");
    if (! aTarget)
        return NS_ERROR_NULL_POINTER;

    if (mReadCount) {
        NS_WARNING("Writing to InMemoryDataSource during read\n");
        return NS_RDF_ASSERTION_REJECTED;
    }

    nsresult rv;

    
    

    rv = LockedUnassert(aOldSource, aProperty, aTarget);
    if (NS_FAILED(rv)) return rv;

    rv = LockedAssert(aNewSource, aProperty, aTarget, true);
    if (NS_FAILED(rv)) return rv;

    
    for (int32_t i = int32_t(mNumObservers) - 1; mPropagateChanges && i >= 0; --i) {
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
    NS_PRECONDITION(aObserver != nullptr, "null ptr");
    if (! aObserver)
        return NS_ERROR_NULL_POINTER;

    mObservers.AppendObject(aObserver);
    mNumObservers = mObservers.Count();

    return NS_OK;
}

NS_IMETHODIMP
InMemoryDataSource::RemoveObserver(nsIRDFObserver* aObserver)
{
    NS_PRECONDITION(aObserver != nullptr, "null ptr");
    if (! aObserver)
        return NS_ERROR_NULL_POINTER;

    mObservers.RemoveObject(aObserver);
    
    
    mNumObservers = mObservers.Count();

    return NS_OK;
}

NS_IMETHODIMP 
InMemoryDataSource::HasArcIn(nsIRDFNode *aNode, nsIRDFResource *aArc, bool *result)
{
    Assertion* ass = GetReverseArcs(aNode);
    while (ass) {
        nsIRDFResource* elbow = ass->u.as.mProperty;
        if (elbow == aArc) {
            *result = true;
            return NS_OK;
        }
        ass = ass->u.as.mInvNext;
    }
    *result = false;
    return NS_OK;
}

NS_IMETHODIMP
InMemoryDataSource::HasArcOut(nsIRDFResource *aSource, nsIRDFResource *aArc, bool *result)
{
    Assertion* ass = GetForwardArcs(aSource);
    if (ass && ass->mHashEntry) {
        PLDHashEntryHdr* hdr =
            PL_DHashTableSearch(ass->u.hash.mPropertyHash, aArc);
        Assertion* val = hdr ? static_cast<Entry*>(hdr)->mAssertions : nullptr;
        if (val) {
            *result = true;
            return NS_OK;
        }
        ass = ass->mNext;
    }
    while (ass) {
        nsIRDFResource* elbow = ass->u.as.mProperty;
        if (elbow == aArc) {
            *result = true;
            return NS_OK;
        }
        ass = ass->mNext;
    }
    *result = false;
    return NS_OK;
}

NS_IMETHODIMP
InMemoryDataSource::ArcLabelsIn(nsIRDFNode* aTarget, nsISimpleEnumerator** aResult)
{
    NS_PRECONDITION(aTarget != nullptr, "null ptr");
    if (! aTarget)
        return NS_ERROR_NULL_POINTER;

    InMemoryArcsEnumeratorImpl* result =
        new InMemoryArcsEnumeratorImpl(this, nullptr, aTarget);

    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(result);
    *aResult = result;

    return NS_OK;
}

NS_IMETHODIMP
InMemoryDataSource::ArcLabelsOut(nsIRDFResource* aSource, nsISimpleEnumerator** aResult)
{
    NS_PRECONDITION(aSource != nullptr, "null ptr");
    if (! aSource)
        return NS_ERROR_NULL_POINTER;

    InMemoryArcsEnumeratorImpl* result =
        new InMemoryArcsEnumeratorImpl(this, aSource, nullptr);

    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(result);
    *aResult = result;

    return NS_OK;
}

PLDHashOperator
InMemoryDataSource::ResourceEnumerator(PLDHashTable* aTable,
                                       PLDHashEntryHdr* aHdr,
                                       uint32_t aNumber, void* aArg)
{
    Entry* entry = static_cast<Entry*>(aHdr);
    static_cast<nsCOMArray<nsIRDFNode>*>(aArg)->AppendObject(entry->mNode);
    return PL_DHASH_NEXT;
}


NS_IMETHODIMP
InMemoryDataSource::GetAllResources(nsISimpleEnumerator** aResult)
{
    nsCOMArray<nsIRDFNode> nodes;
    nodes.SetCapacity(mForwardArcs.EntryCount());

    
    PL_DHashTableEnumerate(&mForwardArcs, ResourceEnumerator, &nodes);

    return NS_NewArrayEnumerator(aResult, nodes);
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
                                     bool* aResult)
{
    *aResult = false;
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
    for (int32_t i = int32_t(mNumObservers) - 1; mPropagateChanges && i >= 0; --i) {
        nsIRDFObserver* obs = mObservers[i];
        obs->OnBeginUpdateBatch(this);
    }
    return NS_OK;
}

NS_IMETHODIMP
InMemoryDataSource::EndUpdateBatch()
{
    for (int32_t i = int32_t(mNumObservers) - 1; mPropagateChanges && i >= 0; --i) {
        nsIRDFObserver* obs = mObservers[i];
        obs->OnEndUpdateBatch(this);
    }
    return NS_OK;
}






NS_IMETHODIMP
InMemoryDataSource::EnsureFastContainment(nsIRDFResource* aSource)
{
    Assertion *as = GetForwardArcs(aSource);
    bool    haveHash = (as) ? as->mHashEntry : false;

    
    if (haveHash)   return(NS_OK);

    
    Assertion *hashAssertion = new Assertion(aSource);
    NS_ASSERTION(hashAssertion, "unable to create Assertion");
    if (!hashAssertion) return(NS_ERROR_OUT_OF_MEMORY);

    
    hashAssertion->AddRef();

    Assertion *first = GetForwardArcs(aSource);
    SetForwardArcs(aSource, hashAssertion);

    
    PLDHashTable *table = hashAssertion->u.hash.mPropertyHash;
    Assertion *nextRef;
    while(first) {
        nextRef = first->mNext;
        nsIRDFResource *prop = first->u.as.mProperty;

        PLDHashEntryHdr* hdr = PL_DHashTableSearch(table, prop);
        Assertion* val = hdr ? static_cast<Entry*>(hdr)->mAssertions : nullptr;
        if (val) {
            first->mNext = val->mNext;
            val->mNext = first;
        }
        else {
            PLDHashEntryHdr* hdr = PL_DHashTableAdd(table, prop,
                                                    mozilla::fallible);
            if (hdr) {
                Entry* entry = static_cast<Entry*>(hdr);
                entry->mNode = prop;
                entry->mAssertions = first;
                first->mNext = nullptr;
            }
        }
        first = nextRef;
    }
    return(NS_OK);
}




NS_IMETHODIMP
InMemoryDataSource::GetPropagateChanges(bool* aPropagateChanges)
{
    *aPropagateChanges = mPropagateChanges;
    return NS_OK;
}

NS_IMETHODIMP
InMemoryDataSource::SetPropagateChanges(bool aPropagateChanges)
{
    mPropagateChanges = aPropagateChanges;
    return NS_OK;
}





NS_IMETHODIMP
InMemoryDataSource::Mark(nsIRDFResource* aSource,
                         nsIRDFResource* aProperty,
                         nsIRDFNode* aTarget,
                         bool aTruthValue,
                         bool* aDidMark)
{
    NS_PRECONDITION(aSource != nullptr, "null ptr");
    if (! aSource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aProperty != nullptr, "null ptr");
    if (! aProperty)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aTarget != nullptr, "null ptr");
    if (! aTarget)
        return NS_ERROR_NULL_POINTER;

    Assertion *as = GetForwardArcs(aSource);
    if (as && as->mHashEntry) {
        PLDHashEntryHdr* hdr =
            PL_DHashTableSearch(as->u.hash.mPropertyHash, aProperty);
        Assertion* val = hdr ? static_cast<Entry*>(hdr)->mAssertions : nullptr;
        while (val) {
            if ((val->u.as.mTarget == aTarget) &&
                (aTruthValue == (val->u.as.mTruthValue))) {

                
                as->Mark();
                *aDidMark = true;

#ifdef PR_LOGGING
                LogOperation("MARK", aSource, aProperty, aTarget, aTruthValue);
#endif

                return NS_OK;
            }
            val = val->mNext;
        }
    }
    else for (; as != nullptr; as = as->mNext) {
        
        if (aTarget != as->u.as.mTarget)
            continue;

        if (aProperty != as->u.as.mProperty)
            continue;

        if (aTruthValue != (as->u.as.mTruthValue))
            continue;

        
        as->Mark();
        *aDidMark = true;

#ifdef PR_LOGGING
        LogOperation("MARK", aSource, aProperty, aTarget, aTruthValue);
#endif

        return NS_OK;
    }

    
    *aDidMark = false;
    return NS_OK;
}


struct SweepInfo {
    Assertion* mUnassertList;
    PLDHashTable* mReverseArcs;
};

NS_IMETHODIMP
InMemoryDataSource::Sweep()
{
    SweepInfo info = { nullptr, &mReverseArcs };

    
    PL_DHashTableEnumerate(&mForwardArcs, SweepForwardArcsEntries, &info);

    
    Assertion* as = info.mUnassertList;
    while (as) {
#ifdef PR_LOGGING
        LogOperation("SWEEP", as->mSource, as->u.as.mProperty, as->u.as.mTarget, as->u.as.mTruthValue);
#endif
        if (!(as->mHashEntry))
        {
            for (int32_t i = int32_t(mNumObservers) - 1; mPropagateChanges && i >= 0; --i) {
                nsIRDFObserver* obs = mObservers[i];
                
                obs->OnUnassert(this, as->mSource, as->u.as.mProperty, as->u.as.mTarget);
                
            }
        }

        Assertion* doomed = as;
        as = as->mNext;

        
        doomed->mNext = doomed->u.as.mInvNext = nullptr;
        doomed->Release();
    }

    return NS_OK;
}


PLDHashOperator
InMemoryDataSource::SweepForwardArcsEntries(PLDHashTable* aTable,
                                            PLDHashEntryHdr* aHdr,
                                            uint32_t aNumber, void* aArg)
{
    PLDHashOperator result = PL_DHASH_NEXT;
    Entry* entry = static_cast<Entry*>(aHdr);
    SweepInfo* info = static_cast<SweepInfo*>(aArg);

    Assertion* as = entry->mAssertions;
    if (as && (as->mHashEntry))
    {
        
        PL_DHashTableEnumerate(as->u.hash.mPropertyHash,
                               SweepForwardArcsEntries, info);

        
        if (!as->u.hash.mPropertyHash->EntryCount()) {
            as->Release();
            result = PL_DHASH_REMOVE;
        }

        return result;
    }

    Assertion* prev = nullptr;
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
                PL_DHashTableSearch(info->mReverseArcs, as->u.as.mTarget);
            NS_ASSERTION(hdr, "no assertion in reverse arcs");

            Entry* rentry = static_cast<Entry*>(hdr);
            Assertion* ras = rentry->mAssertions;
            Assertion* rprev = nullptr;
            while (ras) {
                if (ras == as) {
                    if (rprev) {
                        rprev->u.as.mInvNext = ras->u.as.mInvNext;
                    }
                    else {
                        
                        rentry->mAssertions = ras->u.as.mInvNext;
                    }
                    as->u.as.mInvNext = nullptr; 
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
    explicit VisitorClosure(rdfITripleVisitor* aVisitor) :
        mVisitor(aVisitor),
        mRv(NS_OK)
    {}
    rdfITripleVisitor* mVisitor;
    nsresult mRv;
};

PLDHashOperator
SubjectEnumerator(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
                  uint32_t aNumber, void* aArg) {
    Entry* entry = static_cast<Entry*>(aHdr);
    VisitorClosure* closure = static_cast<VisitorClosure*>(aArg);

    nsresult rv;
    nsCOMPtr<nsIRDFNode> subject = do_QueryInterface(entry->mNode, &rv);
    NS_ENSURE_SUCCESS(rv, PL_DHASH_NEXT);

    closure->mRv = closure->mVisitor->Visit(subject, nullptr, nullptr, true);
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

PLDHashOperator
TriplesInnerEnumerator(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
                  uint32_t aNumber, void* aArg) {
    Entry* entry = static_cast<Entry*>(aHdr);
    Assertion* assertion = entry->mAssertions;
    TriplesInnerClosure* closure = 
        static_cast<TriplesInnerClosure*>(aArg);
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
PLDHashOperator
TriplesEnumerator(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
                  uint32_t aNumber, void* aArg) {
    Entry* entry = static_cast<Entry*>(aHdr);
    VisitorClosure* closure = static_cast<VisitorClosure*>(aArg);

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



