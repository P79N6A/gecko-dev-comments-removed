
































#include "nsRDFService.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsMemory.h"
#include "nsIAtom.h"
#include "nsIComponentManager.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsIServiceManager.h"
#include "nsIFactory.h"
#include "nsRDFCID.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsNetUtil.h"
#include "pldhash.h"
#include "plhash.h"
#include "plstr.h"
#include "prlog.h"
#include "prprf.h"
#include "prmem.h"
#include "rdf.h"
#include "nsCRT.h"
#include "nsCRTGlue.h"
#include "mozilla/HashFunctions.h"

using namespace mozilla;



static NS_DEFINE_CID(kRDFXMLDataSourceCID,    NS_RDFXMLDATASOURCE_CID);
static NS_DEFINE_CID(kRDFDefaultResourceCID,  NS_RDFDEFAULTRESOURCE_CID);

static NS_DEFINE_IID(kIRDFLiteralIID,         NS_IRDFLITERAL_IID);
static NS_DEFINE_IID(kIRDFDateIID,         NS_IRDFDATE_IID);
static NS_DEFINE_IID(kIRDFIntIID,         NS_IRDFINT_IID);
static NS_DEFINE_IID(kIRDFNodeIID,            NS_IRDFNODE_IID);
static NS_DEFINE_IID(kISupportsIID,           NS_ISUPPORTS_IID);

#ifdef PR_LOGGING
static PRLogModuleInfo* gLog = nullptr;
#endif

class BlobImpl;





static void *
DataSourceAllocTable(void *pool, size_t size)
{
    return PR_MALLOC(size);
}

static void
DataSourceFreeTable(void *pool, void *item)
{
    PR_Free(item);
}

static PLHashEntry *
DataSourceAllocEntry(void *pool, const void *key)
{
    return PR_NEW(PLHashEntry);
}

static void
DataSourceFreeEntry(void *pool, PLHashEntry *he, unsigned flag)
{
    if (flag == HT_FREE_ENTRY) {
        PL_strfree((char*) he->key);
        PR_Free(he);
    }
}

static PLHashAllocOps dataSourceHashAllocOps = {
    DataSourceAllocTable, DataSourceFreeTable,
    DataSourceAllocEntry, DataSourceFreeEntry
};






struct ResourceHashEntry : public PLDHashEntryHdr {
    const char *mKey;
    nsIRDFResource *mResource;

    static PLDHashNumber
    HashKey(PLDHashTable *table, const void *key)
    {
        return HashString(static_cast<const char *>(key));
    }

    static bool
    MatchEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
               const void *key)
    {
        const ResourceHashEntry *entry =
            static_cast<const ResourceHashEntry *>(hdr);

        return 0 == nsCRT::strcmp(static_cast<const char *>(key),
                                  entry->mKey);
    }
};

static const PLDHashTableOps gResourceTableOps = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    ResourceHashEntry::HashKey,
    ResourceHashEntry::MatchEntry,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    PL_DHashFinalizeStub,
    nullptr
};






struct LiteralHashEntry : public PLDHashEntryHdr {
    nsIRDFLiteral *mLiteral;
    const char16_t *mKey;

    static PLDHashNumber
    HashKey(PLDHashTable *table, const void *key)
    {
        return HashString(static_cast<const char16_t *>(key));
    }

    static bool
    MatchEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
               const void *key)
    {
        const LiteralHashEntry *entry =
            static_cast<const LiteralHashEntry *>(hdr);

        return 0 == nsCRT::strcmp(static_cast<const char16_t *>(key),
                                  entry->mKey);
    }
};

static const PLDHashTableOps gLiteralTableOps = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    LiteralHashEntry::HashKey,
    LiteralHashEntry::MatchEntry,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    PL_DHashFinalizeStub,
    nullptr
};






struct IntHashEntry : public PLDHashEntryHdr {
    nsIRDFInt *mInt;
    int32_t    mKey;

    static PLDHashNumber
    HashKey(PLDHashTable *table, const void *key)
    {
        return PLDHashNumber(*static_cast<const int32_t *>(key));
    }

    static bool
    MatchEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
               const void *key)
    {
        const IntHashEntry *entry =
            static_cast<const IntHashEntry *>(hdr);

        return *static_cast<const int32_t *>(key) == entry->mKey;
    }
};

static const PLDHashTableOps gIntTableOps = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    IntHashEntry::HashKey,
    IntHashEntry::MatchEntry,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    PL_DHashFinalizeStub,
    nullptr
};






struct DateHashEntry : public PLDHashEntryHdr {
    nsIRDFDate *mDate;
    PRTime      mKey;

    static PLDHashNumber
    HashKey(PLDHashTable *table, const void *key)
    {
        
        PRTime t = *static_cast<const PRTime *>(key);
        int32_t h32 = int32_t(t >> 32);
        int32_t l32 = int32_t(0xffffffff & t);
        return PLDHashNumber(l32 ^ h32);
    }

    static bool
    MatchEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
               const void *key)
    {
        const DateHashEntry *entry =
            static_cast<const DateHashEntry *>(hdr);

        return *static_cast<const PRTime *>(key) == entry->mKey;
    }
};

static const PLDHashTableOps gDateTableOps = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    DateHashEntry::HashKey,
    DateHashEntry::MatchEntry,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    PL_DHashFinalizeStub,
    nullptr
};

class BlobImpl : public nsIRDFBlob
{
public:
    struct Data {
        int32_t  mLength;
        uint8_t *mBytes;
    };

    BlobImpl(const uint8_t *aBytes, int32_t aLength)
    {
        mData.mLength = aLength;
        mData.mBytes = new uint8_t[aLength];
        memcpy(mData.mBytes, aBytes, aLength);
        NS_ADDREF(RDFServiceImpl::gRDFService);
        RDFServiceImpl::gRDFService->RegisterBlob(this);
    }

protected:
    virtual ~BlobImpl()
    {
        RDFServiceImpl::gRDFService->UnregisterBlob(this);
        
        
        
        nsrefcnt refcnt;
        NS_RELEASE2(RDFServiceImpl::gRDFService, refcnt);
        delete[] mData.mBytes;
    }

public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIRDFNODE
    NS_DECL_NSIRDFBLOB

    Data mData;
};

NS_IMPL_ISUPPORTS(BlobImpl, nsIRDFNode, nsIRDFBlob)

NS_IMETHODIMP
BlobImpl::EqualsNode(nsIRDFNode *aNode, bool *aEquals)
{
    nsCOMPtr<nsIRDFBlob> blob = do_QueryInterface(aNode);
    if (blob) {
        int32_t length;
        blob->GetLength(&length);

        if (length == mData.mLength) {
            const uint8_t *bytes;
            blob->GetValue(&bytes);

            if (0 == memcmp(bytes, mData.mBytes, length)) {
                *aEquals = true;
                return NS_OK;
            }
        }
    }

    *aEquals = false;
    return NS_OK;
}

NS_IMETHODIMP
BlobImpl::GetValue(const uint8_t **aResult)
{
    *aResult = mData.mBytes;
    return NS_OK;
}

NS_IMETHODIMP
BlobImpl::GetLength(int32_t *aResult)
{
    *aResult = mData.mLength;
    return NS_OK;
}






struct BlobHashEntry : public PLDHashEntryHdr {
    BlobImpl *mBlob;

    static PLDHashNumber
    HashKey(PLDHashTable *table, const void *key)
    {
        const BlobImpl::Data *data =
            static_cast<const BlobImpl::Data *>(key);
        return HashBytes(data->mBytes, data->mLength);
    }

    static bool
    MatchEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
               const void *key)
    {
        const BlobHashEntry *entry =
            static_cast<const BlobHashEntry *>(hdr);

        const BlobImpl::Data *left = &entry->mBlob->mData;

        const BlobImpl::Data *right =
            static_cast<const BlobImpl::Data *>(key);

        return (left->mLength == right->mLength)
            && 0 == memcmp(left->mBytes, right->mBytes, right->mLength);
    }
};

static const PLDHashTableOps gBlobTableOps = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    BlobHashEntry::HashKey,
    BlobHashEntry::MatchEntry,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    PL_DHashFinalizeStub,
    nullptr
};








class LiteralImpl : public nsIRDFLiteral {
public:
    static nsresult
    Create(const char16_t* aValue, nsIRDFLiteral** aResult);

    
    NS_DECL_THREADSAFE_ISUPPORTS

    
    NS_DECL_NSIRDFNODE

    
    NS_DECL_NSIRDFLITERAL

protected:
    explicit LiteralImpl(const char16_t* s);
    virtual ~LiteralImpl();

    const char16_t* GetValue() const {
        size_t objectSize = ((sizeof(LiteralImpl) + sizeof(char16_t) - 1) / sizeof(char16_t)) * sizeof(char16_t);
        return reinterpret_cast<const char16_t*>(reinterpret_cast<const unsigned char*>(this) + objectSize);
    }
};


nsresult
LiteralImpl::Create(const char16_t* aValue, nsIRDFLiteral** aResult)
{
    
    size_t objectSize = ((sizeof(LiteralImpl) + sizeof(char16_t) - 1) / sizeof(char16_t)) * sizeof(char16_t);
    size_t stringLen = nsCharTraits<char16_t>::length(aValue);
    size_t stringSize = (stringLen + 1) * sizeof(char16_t);

    void* objectPtr = operator new(objectSize + stringSize);
    if (! objectPtr)
        return NS_ERROR_NULL_POINTER;

    char16_t* buf = reinterpret_cast<char16_t*>(static_cast<unsigned char*>(objectPtr) + objectSize);
    nsCharTraits<char16_t>::copy(buf, aValue, stringLen + 1);

    NS_ADDREF(*aResult = new (objectPtr) LiteralImpl(buf));
    return NS_OK;
}


LiteralImpl::LiteralImpl(const char16_t* s)
{
    RDFServiceImpl::gRDFService->RegisterLiteral(this);
    NS_ADDREF(RDFServiceImpl::gRDFService);
}

LiteralImpl::~LiteralImpl()
{
    RDFServiceImpl::gRDFService->UnregisterLiteral(this);

    
    
    
    nsrefcnt refcnt;
    NS_RELEASE2(RDFServiceImpl::gRDFService, refcnt);
}

NS_IMPL_ADDREF(LiteralImpl)
NS_IMPL_RELEASE(LiteralImpl)

nsresult
LiteralImpl::QueryInterface(REFNSIID iid, void** result)
{
    if (! result)
        return NS_ERROR_NULL_POINTER;

    *result = nullptr;
    if (iid.Equals(kIRDFLiteralIID) ||
        iid.Equals(kIRDFNodeIID) ||
        iid.Equals(kISupportsIID)) {
        *result = static_cast<nsIRDFLiteral*>(this);
        AddRef();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}

NS_IMETHODIMP
LiteralImpl::EqualsNode(nsIRDFNode* aNode, bool* aResult)
{
    nsresult rv;
    nsIRDFLiteral* literal;
    rv = aNode->QueryInterface(kIRDFLiteralIID, (void**) &literal);
    if (NS_SUCCEEDED(rv)) {
        *aResult = (static_cast<nsIRDFLiteral*>(this) == literal);
        NS_RELEASE(literal);
        return NS_OK;
    }
    else if (rv == NS_NOINTERFACE) {
        *aResult = false;
        return NS_OK;
    }
    else {
        return rv;
    }
}

NS_IMETHODIMP
LiteralImpl::GetValue(char16_t* *value)
{
    NS_ASSERTION(value, "null ptr");
    if (! value)
        return NS_ERROR_NULL_POINTER;

    const char16_t *temp = GetValue();
    *value = temp? NS_strdup(temp) : 0;
    return NS_OK;
}


NS_IMETHODIMP
LiteralImpl::GetValueConst(const char16_t** aValue)
{
    *aValue = GetValue();
    return NS_OK;
}





class DateImpl : public nsIRDFDate {
public:
    explicit DateImpl(const PRTime s);

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIRDFNODE

    
    NS_IMETHOD GetValue(PRTime *value);

private:
    virtual ~DateImpl();

    nsresult EqualsDate(nsIRDFDate* date, bool* result);
    PRTime mValue;
};


DateImpl::DateImpl(const PRTime s)
    : mValue(s)
{
    RDFServiceImpl::gRDFService->RegisterDate(this);
    NS_ADDREF(RDFServiceImpl::gRDFService);
}

DateImpl::~DateImpl()
{
    RDFServiceImpl::gRDFService->UnregisterDate(this);

    
    
    
    nsrefcnt refcnt;
    NS_RELEASE2(RDFServiceImpl::gRDFService, refcnt);
}

NS_IMPL_ADDREF(DateImpl)
NS_IMPL_RELEASE(DateImpl)

nsresult
DateImpl::QueryInterface(REFNSIID iid, void** result)
{
    if (! result)
        return NS_ERROR_NULL_POINTER;

    *result = nullptr;
    if (iid.Equals(kIRDFDateIID) ||
        iid.Equals(kIRDFNodeIID) ||
        iid.Equals(kISupportsIID)) {
        *result = static_cast<nsIRDFDate*>(this);
        AddRef();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}

NS_IMETHODIMP
DateImpl::EqualsNode(nsIRDFNode* node, bool* result)
{
    nsresult rv;
    nsIRDFDate* date;
    if (NS_SUCCEEDED(node->QueryInterface(kIRDFDateIID, (void**) &date))) {
        rv = EqualsDate(date, result);
        NS_RELEASE(date);
    }
    else {
        *result = false;
        rv = NS_OK;
    }
    return rv;
}

NS_IMETHODIMP
DateImpl::GetValue(PRTime *value)
{
    NS_ASSERTION(value, "null ptr");
    if (! value)
        return NS_ERROR_NULL_POINTER;

    *value = mValue;
    return NS_OK;
}


nsresult
DateImpl::EqualsDate(nsIRDFDate* date, bool* result)
{
    NS_ASSERTION(date && result, "null ptr");
    if (!date || !result)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;
    PRTime p;
    if (NS_FAILED(rv = date->GetValue(&p)))
        return rv;

    *result = p == mValue;
    return NS_OK;
}





class IntImpl : public nsIRDFInt {
public:
    explicit IntImpl(int32_t s);

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIRDFNODE

    
    NS_IMETHOD GetValue(int32_t *value);

private:
    virtual ~IntImpl();

    nsresult EqualsInt(nsIRDFInt* value, bool* result);
    int32_t mValue;
};


IntImpl::IntImpl(int32_t s)
    : mValue(s)
{
    RDFServiceImpl::gRDFService->RegisterInt(this);
    NS_ADDREF(RDFServiceImpl::gRDFService);
}

IntImpl::~IntImpl()
{
    RDFServiceImpl::gRDFService->UnregisterInt(this);

    
    
    
    nsrefcnt refcnt;
    NS_RELEASE2(RDFServiceImpl::gRDFService, refcnt);
}

NS_IMPL_ADDREF(IntImpl)
NS_IMPL_RELEASE(IntImpl)

nsresult
IntImpl::QueryInterface(REFNSIID iid, void** result)
{
    if (! result)
        return NS_ERROR_NULL_POINTER;

    *result = nullptr;
    if (iid.Equals(kIRDFIntIID) ||
        iid.Equals(kIRDFNodeIID) ||
        iid.Equals(kISupportsIID)) {
        *result = static_cast<nsIRDFInt*>(this);
        AddRef();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}

NS_IMETHODIMP
IntImpl::EqualsNode(nsIRDFNode* node, bool* result)
{
    nsresult rv;
    nsIRDFInt* intValue;
    if (NS_SUCCEEDED(node->QueryInterface(kIRDFIntIID, (void**) &intValue))) {
        rv = EqualsInt(intValue, result);
        NS_RELEASE(intValue);
    }
    else {
        *result = false;
        rv = NS_OK;
    }
    return rv;
}

NS_IMETHODIMP
IntImpl::GetValue(int32_t *value)
{
    NS_ASSERTION(value, "null ptr");
    if (! value)
        return NS_ERROR_NULL_POINTER;

    *value = mValue;
    return NS_OK;
}


nsresult
IntImpl::EqualsInt(nsIRDFInt* intValue, bool* result)
{
    NS_ASSERTION(intValue && result, "null ptr");
    if (!intValue || !result)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;
    int32_t p;
    if (NS_FAILED(rv = intValue->GetValue(&p)))
        return rv;

    *result = (p == mValue);
    return NS_OK;
}




RDFServiceImpl*
RDFServiceImpl::gRDFService;

RDFServiceImpl::RDFServiceImpl()
    :  mNamedDataSources(nullptr)
{
    mResources.ops = nullptr;
    mLiterals.ops = nullptr;
    mInts.ops = nullptr;
    mDates.ops = nullptr;
    mBlobs.ops = nullptr;
    gRDFService = this;
}

nsresult
RDFServiceImpl::Init()
{
    nsresult rv;

    mNamedDataSources = PL_NewHashTable(23,
                                        PL_HashString,
                                        PL_CompareStrings,
                                        PL_CompareValues,
                                        &dataSourceHashAllocOps, nullptr);

    if (! mNamedDataSources)
        return NS_ERROR_OUT_OF_MEMORY;

    PL_DHashTableInit(&mResources, &gResourceTableOps, nullptr,
                      sizeof(ResourceHashEntry));

    PL_DHashTableInit(&mLiterals, &gLiteralTableOps, nullptr,
                      sizeof(LiteralHashEntry));

    PL_DHashTableInit(&mInts, &gIntTableOps, nullptr,
                      sizeof(IntHashEntry));

    PL_DHashTableInit(&mDates, &gDateTableOps, nullptr,
                      sizeof(DateHashEntry));

    PL_DHashTableInit(&mBlobs, &gBlobTableOps, nullptr,
                      sizeof(BlobHashEntry));

    mDefaultResourceFactory = do_GetClassObject(kRDFDefaultResourceCID, &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get default resource factory");
    if (NS_FAILED(rv)) return rv;

#ifdef PR_LOGGING
    if (! gLog)
        gLog = PR_NewLogModule("nsRDFService");
#endif

    return NS_OK;
}


RDFServiceImpl::~RDFServiceImpl()
{
    if (mNamedDataSources) {
        PL_HashTableDestroy(mNamedDataSources);
        mNamedDataSources = nullptr;
    }
    if (mResources.ops)
        PL_DHashTableFinish(&mResources);
    if (mLiterals.ops)
        PL_DHashTableFinish(&mLiterals);
    if (mInts.ops)
        PL_DHashTableFinish(&mInts);
    if (mDates.ops)
        PL_DHashTableFinish(&mDates);
    if (mBlobs.ops)
        PL_DHashTableFinish(&mBlobs);
    gRDFService = nullptr;
}



nsresult
RDFServiceImpl::CreateSingleton(nsISupports* aOuter,
                                const nsIID& aIID, void **aResult)
{
    NS_ENSURE_NO_AGGREGATION(aOuter);

    if (gRDFService) {
        NS_ERROR("Trying to create RDF serviec twice.");
        return gRDFService->QueryInterface(aIID, aResult);
    }

    nsRefPtr<RDFServiceImpl> serv = new RDFServiceImpl();
    if (!serv)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = serv->Init();
    if (NS_FAILED(rv))
        return rv;

    return serv->QueryInterface(aIID, aResult);
}

NS_IMPL_ISUPPORTS(RDFServiceImpl, nsIRDFService, nsISupportsWeakReference)


static const uint8_t
kLegalSchemeChars[] = {
          
          
    0x00, 
    0x00, 
    0x00, 
    0x00, 
    0x00, 
    0x28, 
    0xff, 
    0x03, 
    0xfe, 
    0xff, 
    0xff, 
    0x87, 
    0xfe, 
    0xff, 
    0xff, 
    0x07, 
    0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

static inline bool
IsLegalSchemeCharacter(const char aChar)
{
    uint8_t mask = kLegalSchemeChars[aChar >> 3];
    uint8_t bit = 1u << (aChar & 0x7);
    return bool((mask & bit) != 0);
}


NS_IMETHODIMP
RDFServiceImpl::GetResource(const nsACString& aURI, nsIRDFResource** aResource)
{
    
    NS_PRECONDITION(aResource != nullptr, "null ptr");
    NS_PRECONDITION(!aURI.IsEmpty(), "URI is empty");
    if (! aResource)
        return NS_ERROR_NULL_POINTER;
    if (aURI.IsEmpty())
        return NS_ERROR_INVALID_ARG;

    const nsAFlatCString& flatURI = PromiseFlatCString(aURI);
    PR_LOG(gLog, PR_LOG_DEBUG, ("rdfserv get-resource %s", flatURI.get()));

    
    
    PLDHashEntryHdr *hdr =
        PL_DHashTableOperate(&mResources, flatURI.get(), PL_DHASH_LOOKUP);

    if (PL_DHASH_ENTRY_IS_BUSY(hdr)) {
        ResourceHashEntry *entry = static_cast<ResourceHashEntry *>(hdr);
        NS_ADDREF(*aResource = entry->mResource);
        return NS_OK;
    }

    

    
    
    
    
    
    
    
    
    
    
    
    nsACString::const_iterator p, end;
    aURI.BeginReading(p);
    aURI.EndReading(end);
    while (p != end && IsLegalSchemeCharacter(*p))
        ++p;

    nsresult rv;
    nsCOMPtr<nsIFactory> factory;

    nsACString::const_iterator begin;
    aURI.BeginReading(begin);
    if (*p == ':') {
        
        
        if (mLastFactory && mLastURIPrefix.Equals(Substring(begin, p)))
            factory = mLastFactory;
        else {
            
            nsACString::const_iterator begin;
            aURI.BeginReading(begin);
            nsAutoCString contractID;
            contractID = NS_LITERAL_CSTRING(NS_RDF_RESOURCE_FACTORY_CONTRACTID_PREFIX) +
                         Substring(begin, p);

            factory = do_GetClassObject(contractID.get());
            if (factory) {
                
                if (p != begin) {
                    mLastFactory = factory;
                    mLastURIPrefix = Substring(begin, p);
                }
            }
        }
    }

    if (! factory) {
        
        
        
        
        factory = mDefaultResourceFactory;

        
        if (p != begin) {
            mLastFactory = factory;
            mLastURIPrefix = Substring(begin, p);
        }
    }

    nsIRDFResource *result;
    rv = factory->CreateInstance(nullptr, NS_GET_IID(nsIRDFResource), (void**) &result);
    if (NS_FAILED(rv)) return rv;

    
    
    rv = result->Init(flatURI.get());
    if (NS_FAILED(rv)) {
        NS_ERROR("unable to initialize resource");
        NS_RELEASE(result);
        return rv;
    }

    *aResource = result; 
    return rv;
}

NS_IMETHODIMP
RDFServiceImpl::GetUnicodeResource(const nsAString& aURI, nsIRDFResource** aResource)
{
    return GetResource(NS_ConvertUTF16toUTF8(aURI), aResource);
}


NS_IMETHODIMP
RDFServiceImpl::GetAnonymousResource(nsIRDFResource** aResult)
{
static uint32_t gCounter = 0;
static char gChars[] = "0123456789abcdef"
                       "ghijklmnopqrstuv"
                       "wxyzABCDEFGHIJKL"
                       "MNOPQRSTUVWXYZ.+";

static int32_t kMask  = 0x003f;
static int32_t kShift = 6;

    if (! gCounter) {
        
        
        
        
        
        
        
        
        gCounter = uint32_t(PR_Now());
    }

    nsresult rv;
    nsAutoCString s;

    do {
        
        
        

        s.Truncate();
        s.AppendLiteral("rdf:#$");

        uint32_t id = ++gCounter;
        while (id) {
            char ch = gChars[(id & kMask)];
            s.Append(ch);
            id >>= kShift;
        }

        nsIRDFResource* resource;
        rv = GetResource(s, &resource);
        if (NS_FAILED(rv)) return rv;

        
        
        resource->AddRef();
        nsrefcnt refcnt = resource->Release();

        if (refcnt == 1) {
            *aResult = resource;
            break;
        }

        NS_RELEASE(resource);
    } while (1);

    return NS_OK;
}


NS_IMETHODIMP
RDFServiceImpl::GetLiteral(const char16_t* aValue, nsIRDFLiteral** aLiteral)
{
    NS_PRECONDITION(aValue != nullptr, "null ptr");
    if (! aValue)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aLiteral != nullptr, "null ptr");
    if (! aLiteral)
        return NS_ERROR_NULL_POINTER;

    
    PLDHashEntryHdr *hdr =
        PL_DHashTableOperate(&mLiterals, aValue, PL_DHASH_LOOKUP);

    if (PL_DHASH_ENTRY_IS_BUSY(hdr)) {
        LiteralHashEntry *entry = static_cast<LiteralHashEntry *>(hdr);
        NS_ADDREF(*aLiteral = entry->mLiteral);
        return NS_OK;
    }

    
    return LiteralImpl::Create(aValue, aLiteral);
}

NS_IMETHODIMP
RDFServiceImpl::GetDateLiteral(PRTime aTime, nsIRDFDate** aResult)
{
    
    PLDHashEntryHdr *hdr =
        PL_DHashTableOperate(&mDates, &aTime, PL_DHASH_LOOKUP);

    if (PL_DHASH_ENTRY_IS_BUSY(hdr)) {
        DateHashEntry *entry = static_cast<DateHashEntry *>(hdr);
        NS_ADDREF(*aResult = entry->mDate);
        return NS_OK;
    }

    DateImpl* result = new DateImpl(aTime);
    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aResult = result);
    return NS_OK;
}

NS_IMETHODIMP
RDFServiceImpl::GetIntLiteral(int32_t aInt, nsIRDFInt** aResult)
{
    
    PLDHashEntryHdr *hdr =
        PL_DHashTableOperate(&mInts, &aInt, PL_DHASH_LOOKUP);

    if (PL_DHASH_ENTRY_IS_BUSY(hdr)) {
        IntHashEntry *entry = static_cast<IntHashEntry *>(hdr);
        NS_ADDREF(*aResult = entry->mInt);
        return NS_OK;
    }

    IntImpl* result = new IntImpl(aInt);
    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aResult = result);
    return NS_OK;
}

NS_IMETHODIMP
RDFServiceImpl::GetBlobLiteral(const uint8_t *aBytes, int32_t aLength,
                               nsIRDFBlob **aResult)
{
    BlobImpl::Data key = { aLength, const_cast<uint8_t *>(aBytes) };

    PLDHashEntryHdr *hdr =
        PL_DHashTableOperate(&mBlobs, &key, PL_DHASH_LOOKUP);

    if (PL_DHASH_ENTRY_IS_BUSY(hdr)) {
        BlobHashEntry *entry = static_cast<BlobHashEntry *>(hdr);
        NS_ADDREF(*aResult = entry->mBlob);
        return NS_OK;
    }

    BlobImpl *result = new BlobImpl(aBytes, aLength);
    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aResult = result);
    return NS_OK;
}

NS_IMETHODIMP
RDFServiceImpl::IsAnonymousResource(nsIRDFResource* aResource, bool* _result)
{
    NS_PRECONDITION(aResource != nullptr, "null ptr");
    if (! aResource)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

    const char* uri;
    rv = aResource->GetValueConst(&uri);
    if (NS_FAILED(rv)) return rv;

    if ((uri[0] == 'r') &&
        (uri[1] == 'd') &&
        (uri[2] == 'f') &&
        (uri[3] == ':') &&
        (uri[4] == '#') &&
        (uri[5] == '$')) {
        *_result = true;
    }
    else {
        *_result = false;
    }

    return NS_OK;
}

NS_IMETHODIMP
RDFServiceImpl::RegisterResource(nsIRDFResource* aResource, bool aReplace)
{
    NS_PRECONDITION(aResource != nullptr, "null ptr");
    if (! aResource)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

    const char* uri;
    rv = aResource->GetValueConst(&uri);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get URI from resource");
    if (NS_FAILED(rv)) return rv;

    NS_ASSERTION(uri != nullptr, "resource has no URI");
    if (! uri)
        return NS_ERROR_NULL_POINTER;

    PLDHashEntryHdr *hdr =
        PL_DHashTableOperate(&mResources, uri, PL_DHASH_LOOKUP);

    if (PL_DHASH_ENTRY_IS_BUSY(hdr)) {
        if (!aReplace) {
            NS_WARNING("resource already registered, and replace not specified");
            return NS_ERROR_FAILURE;    
        }

        
        
        

        PR_LOG(gLog, PR_LOG_DEBUG,
               ("rdfserv   replace-resource [%p] <-- [%p] %s",
                static_cast<ResourceHashEntry *>(hdr)->mResource,
                aResource, (const char*) uri));
    }
    else {
        hdr = PL_DHashTableOperate(&mResources, uri, PL_DHASH_ADD);
        if (! hdr)
            return NS_ERROR_OUT_OF_MEMORY;

        PR_LOG(gLog, PR_LOG_DEBUG,
               ("rdfserv   register-resource [%p] %s",
                aResource, (const char*) uri));
    }

    
    
    
    
    ResourceHashEntry *entry = static_cast<ResourceHashEntry *>(hdr);
    entry->mResource = aResource;
    entry->mKey = uri;

    return NS_OK;
}

NS_IMETHODIMP
RDFServiceImpl::UnregisterResource(nsIRDFResource* aResource)
{
    NS_PRECONDITION(aResource != nullptr, "null ptr");
    if (! aResource)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

    const char* uri;
    rv = aResource->GetValueConst(&uri);
    if (NS_FAILED(rv)) return rv;

    NS_ASSERTION(uri != nullptr, "resource has no URI");
    if (! uri)
        return NS_ERROR_UNEXPECTED;

    PR_LOG(gLog, PR_LOG_DEBUG,
           ("rdfserv unregister-resource [%p] %s",
            aResource, (const char*) uri));

#ifdef DEBUG
    if (PL_DHASH_ENTRY_IS_FREE(PL_DHashTableOperate(&mResources, uri, PL_DHASH_LOOKUP)))
        NS_WARNING("resource was never registered");
#endif

    PL_DHashTableOperate(&mResources, uri, PL_DHASH_REMOVE);
    return NS_OK;
}

NS_IMETHODIMP
RDFServiceImpl::RegisterDataSource(nsIRDFDataSource* aDataSource, bool aReplace)
{
    NS_PRECONDITION(aDataSource != nullptr, "null ptr");
    if (! aDataSource)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

    nsXPIDLCString uri;
    rv = aDataSource->GetURI(getter_Copies(uri));
    if (NS_FAILED(rv)) return rv;

    PLHashEntry** hep =
        PL_HashTableRawLookup(mNamedDataSources, (*mNamedDataSources->keyHash)(uri), uri);

    if (*hep) {
        if (! aReplace)
            return NS_ERROR_FAILURE; 

        
        
        
        PR_LOG(gLog, PR_LOG_NOTICE,
               ("rdfserv    replace-datasource [%p] <-- [%p] %s",
                (*hep)->value, aDataSource, (const char*) uri));

        (*hep)->value = aDataSource;
    }
    else {
        const char* key = PL_strdup(uri);
        if (! key)
            return NS_ERROR_OUT_OF_MEMORY;

        PL_HashTableAdd(mNamedDataSources, key, aDataSource);

        PR_LOG(gLog, PR_LOG_NOTICE,
               ("rdfserv   register-datasource [%p] %s",
                aDataSource, (const char*) uri));

        
        
    }

    return NS_OK;
}

NS_IMETHODIMP
RDFServiceImpl::UnregisterDataSource(nsIRDFDataSource* aDataSource)
{
    NS_PRECONDITION(aDataSource != nullptr, "null ptr");
    if (! aDataSource)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

    nsXPIDLCString uri;
    rv = aDataSource->GetURI(getter_Copies(uri));
    if (NS_FAILED(rv)) return rv;

    
    if (! uri)
        return NS_ERROR_UNEXPECTED;

    PLHashEntry** hep =
        PL_HashTableRawLookup(mNamedDataSources, (*mNamedDataSources->keyHash)(uri), uri);

    
    
    if (! *hep || ((*hep)->value != aDataSource))
        return NS_OK;

    
    
    PL_HashTableRawRemove(mNamedDataSources, hep, *hep);

    PR_LOG(gLog, PR_LOG_NOTICE,
           ("rdfserv unregister-datasource [%p] %s",
            aDataSource, (const char*) uri));

    return NS_OK;
}

NS_IMETHODIMP
RDFServiceImpl::GetDataSource(const char* aURI, nsIRDFDataSource** aDataSource)
{
    
    
    
    return GetDataSource( aURI, false, aDataSource );
}

NS_IMETHODIMP
RDFServiceImpl::GetDataSourceBlocking(const char* aURI, nsIRDFDataSource** aDataSource)
{
    
    return GetDataSource( aURI, true, aDataSource );
}

nsresult
RDFServiceImpl::GetDataSource(const char* aURI, bool aBlock, nsIRDFDataSource** aDataSource)
{
    NS_PRECONDITION(aURI != nullptr, "null ptr");
    if (! aURI)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

    
    
    
    nsAutoCString spec(aURI);

    if (!StringBeginsWith(spec, NS_LITERAL_CSTRING("rdf:"))) {
        nsCOMPtr<nsIURI> uri;
        NS_NewURI(getter_AddRefs(uri), spec);
        if (uri)
            uri->GetSpec(spec);
    }

    
    
    {
        nsIRDFDataSource* cached =
            static_cast<nsIRDFDataSource*>(PL_HashTableLookup(mNamedDataSources, spec.get()));

        if (cached) {
            NS_ADDREF(cached);
            *aDataSource = cached;
            return NS_OK;
        }
    }

    
    nsCOMPtr<nsIRDFDataSource> ds;
    if (StringBeginsWith(spec, NS_LITERAL_CSTRING("rdf:"))) {
        
        nsAutoCString contractID(
                NS_LITERAL_CSTRING(NS_RDF_DATASOURCE_CONTRACTID_PREFIX) +
                Substring(spec, 4, spec.Length() - 4));

        
        int32_t p = contractID.FindChar(char16_t('&'));
        if (p >= 0)
            contractID.Truncate(p);

        ds = do_GetService(contractID.get(), &rv);
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIRDFRemoteDataSource> remote = do_QueryInterface(ds);
        if (remote) {
            rv = remote->Init(spec.get());
            if (NS_FAILED(rv)) return rv;
        }
    }
    else {
        
        ds = do_CreateInstance(kRDFXMLDataSourceCID, &rv);
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIRDFRemoteDataSource> remote(do_QueryInterface(ds));
        NS_ASSERTION(remote, "not a remote RDF/XML data source!");
        if (! remote) return NS_ERROR_UNEXPECTED;

        rv = remote->Init(spec.get());
        if (NS_FAILED(rv)) return rv;

        rv = remote->Refresh(aBlock);
        if (NS_FAILED(rv)) return rv;
    }

    *aDataSource = ds;
    NS_ADDREF(*aDataSource);
    return NS_OK;
}



nsresult
RDFServiceImpl::RegisterLiteral(nsIRDFLiteral* aLiteral)
{
    const char16_t* value;
    aLiteral->GetValueConst(&value);

    NS_ASSERTION(PL_DHASH_ENTRY_IS_FREE(PL_DHashTableOperate(&mLiterals,
                                                             value,
                                                             PL_DHASH_LOOKUP)),
                 "literal already registered");

    PLDHashEntryHdr *hdr =
        PL_DHashTableOperate(&mLiterals, value, PL_DHASH_ADD);

    if (! hdr)
        return NS_ERROR_OUT_OF_MEMORY;

    LiteralHashEntry *entry = static_cast<LiteralHashEntry *>(hdr);

    
    
    
    
    entry->mLiteral = aLiteral;
    entry->mKey = value;

    PR_LOG(gLog, PR_LOG_DEBUG,
           ("rdfserv   register-literal [%p] %s",
            aLiteral, (const char16_t*) value));

    return NS_OK;
}


nsresult
RDFServiceImpl::UnregisterLiteral(nsIRDFLiteral* aLiteral)
{
    const char16_t* value;
    aLiteral->GetValueConst(&value);

    NS_ASSERTION(PL_DHASH_ENTRY_IS_BUSY(PL_DHashTableOperate(&mLiterals,
                                                             value,
                                                             PL_DHASH_LOOKUP)),
                 "literal was never registered");

    PL_DHashTableOperate(&mLiterals, value, PL_DHASH_REMOVE);

    
    
    PR_LOG(gLog, PR_LOG_DEBUG,
           ("rdfserv unregister-literal [%p] %s",
            aLiteral, (const char16_t*) value));

    return NS_OK;
}



nsresult
RDFServiceImpl::RegisterInt(nsIRDFInt* aInt)
{
    int32_t value;
    aInt->GetValue(&value);

    NS_ASSERTION(PL_DHASH_ENTRY_IS_FREE(PL_DHashTableOperate(&mInts,
                                                             &value,
                                                             PL_DHASH_LOOKUP)),
                 "int already registered");

    PLDHashEntryHdr *hdr =
        PL_DHashTableOperate(&mInts, &value, PL_DHASH_ADD);

    if (! hdr)
        return NS_ERROR_OUT_OF_MEMORY;

    IntHashEntry *entry = static_cast<IntHashEntry *>(hdr);

    
    
    
    
    entry->mInt = aInt;
    entry->mKey = value;

    PR_LOG(gLog, PR_LOG_DEBUG,
           ("rdfserv   register-int [%p] %d",
            aInt, value));

    return NS_OK;
}


nsresult
RDFServiceImpl::UnregisterInt(nsIRDFInt* aInt)
{
    int32_t value;
    aInt->GetValue(&value);

    NS_ASSERTION(PL_DHASH_ENTRY_IS_BUSY(PL_DHashTableOperate(&mInts,
                                                             &value,
                                                             PL_DHASH_LOOKUP)),
                 "int was never registered");

    PL_DHashTableOperate(&mInts, &value, PL_DHASH_REMOVE);

    
    
    PR_LOG(gLog, PR_LOG_DEBUG,
           ("rdfserv unregister-int [%p] %d",
            aInt, value));

    return NS_OK;
}



nsresult
RDFServiceImpl::RegisterDate(nsIRDFDate* aDate)
{
    PRTime value;
    aDate->GetValue(&value);

    NS_ASSERTION(PL_DHASH_ENTRY_IS_FREE(PL_DHashTableOperate(&mDates,
                                                             &value,
                                                             PL_DHASH_LOOKUP)),
                 "date already registered");

    PLDHashEntryHdr *hdr =
        PL_DHashTableOperate(&mDates, &value, PL_DHASH_ADD);

    if (! hdr)
        return NS_ERROR_OUT_OF_MEMORY;

    DateHashEntry *entry = static_cast<DateHashEntry *>(hdr);

    
    
    
    
    entry->mDate = aDate;
    entry->mKey = value;

    PR_LOG(gLog, PR_LOG_DEBUG,
           ("rdfserv   register-date [%p] %ld",
            aDate, value));

    return NS_OK;
}


nsresult
RDFServiceImpl::UnregisterDate(nsIRDFDate* aDate)
{
    PRTime value;
    aDate->GetValue(&value);

    NS_ASSERTION(PL_DHASH_ENTRY_IS_BUSY(PL_DHashTableOperate(&mDates,
                                                             &value,
                                                             PL_DHASH_LOOKUP)),
                 "date was never registered");

    PL_DHashTableOperate(&mDates, &value, PL_DHASH_REMOVE);

    
    
    PR_LOG(gLog, PR_LOG_DEBUG,
           ("rdfserv unregister-date [%p] %ld",
            aDate, value));

    return NS_OK;
}

nsresult
RDFServiceImpl::RegisterBlob(BlobImpl *aBlob)
{
    NS_ASSERTION(PL_DHASH_ENTRY_IS_FREE(PL_DHashTableOperate(&mBlobs,
                                                             &aBlob->mData,
                                                             PL_DHASH_LOOKUP)),
                 "blob already registered");

    PLDHashEntryHdr *hdr = 
        PL_DHashTableOperate(&mBlobs, &aBlob->mData, PL_DHASH_ADD);

    if (! hdr)
        return NS_ERROR_OUT_OF_MEMORY;

    BlobHashEntry *entry = static_cast<BlobHashEntry *>(hdr);

    
    
    
    
    entry->mBlob = aBlob;

    PR_LOG(gLog, PR_LOG_DEBUG,
           ("rdfserv   register-blob [%p] %s",
            aBlob, aBlob->mData.mBytes));

    return NS_OK;
}

nsresult
RDFServiceImpl::UnregisterBlob(BlobImpl *aBlob)
{
    NS_ASSERTION(PL_DHASH_ENTRY_IS_BUSY(PL_DHashTableOperate(&mBlobs,
                                                             &aBlob->mData,
                                                             PL_DHASH_LOOKUP)),
                 "blob was never registered");

    PL_DHashTableOperate(&mBlobs, &aBlob->mData, PL_DHASH_REMOVE);
 
     
     
    PR_LOG(gLog, PR_LOG_DEBUG,
           ("rdfserv unregister-blob [%p] %s",
            aBlob, aBlob->mData.mBytes));

    return NS_OK;
}
