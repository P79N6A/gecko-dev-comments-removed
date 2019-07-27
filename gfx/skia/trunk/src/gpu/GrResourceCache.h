









#ifndef GrResourceCache_DEFINED
#define GrResourceCache_DEFINED

#include "GrConfig.h"
#include "GrTypes.h"
#include "SkTMultiMap.h"
#include "GrBinHashKey.h"
#include "SkMessageBus.h"
#include "SkTInternalLList.h"

class GrGpuResource;
class GrResourceCache;
class GrResourceCacheEntry;

class GrResourceKey {
public:
    static GrCacheID::Domain ScratchDomain() {
        static const GrCacheID::Domain gDomain = GrCacheID::GenerateDomain();
        return gDomain;
    }

    

    typedef uint8_t ResourceType;

    
    typedef uint8_t ResourceFlags;

    
    static ResourceType GenerateResourceType();

    
    GrResourceKey(const GrCacheID& id, ResourceType type, ResourceFlags flags) {
        this->init(id.getDomain(), id.getKey(), type, flags);
    };

    GrResourceKey(const GrResourceKey& src) {
        fKey = src.fKey;
    }

    GrResourceKey() {
        fKey.reset();
    }

    void reset(const GrCacheID& id, ResourceType type, ResourceFlags flags) {
        this->init(id.getDomain(), id.getKey(), type, flags);
    }

    uint32_t getHash() const {
        return fKey.getHash();
    }

    bool isScratch() const {
        return ScratchDomain() ==
            *reinterpret_cast<const GrCacheID::Domain*>(fKey.getData() +
                                                        kCacheIDDomainOffset);
    }

    ResourceType getResourceType() const {
        return *reinterpret_cast<const ResourceType*>(fKey.getData() +
                                                      kResourceTypeOffset);
    }

    ResourceFlags getResourceFlags() const {
        return *reinterpret_cast<const ResourceFlags*>(fKey.getData() +
                                                       kResourceFlagsOffset);
    }

    bool operator==(const GrResourceKey& other) const { return fKey == other.fKey; }

private:
    enum {
        kCacheIDKeyOffset = 0,
        kCacheIDDomainOffset = kCacheIDKeyOffset + sizeof(GrCacheID::Key),
        kResourceTypeOffset = kCacheIDDomainOffset + sizeof(GrCacheID::Domain),
        kResourceFlagsOffset = kResourceTypeOffset + sizeof(ResourceType),
        kPadOffset = kResourceFlagsOffset + sizeof(ResourceFlags),
        kKeySize = SkAlign4(kPadOffset),
        kPadSize = kKeySize - kPadOffset
    };

    void init(const GrCacheID::Domain domain,
              const GrCacheID::Key& key,
              ResourceType type,
              ResourceFlags flags) {
        union {
            uint8_t  fKey8[kKeySize];
            uint32_t fKey32[kKeySize / 4];
        } keyData;

        uint8_t* k = keyData.fKey8;
        memcpy(k + kCacheIDKeyOffset, key.fData8, sizeof(GrCacheID::Key));
        memcpy(k + kCacheIDDomainOffset, &domain, sizeof(GrCacheID::Domain));
        memcpy(k + kResourceTypeOffset, &type, sizeof(ResourceType));
        memcpy(k + kResourceFlagsOffset, &flags, sizeof(ResourceFlags));
        memset(k + kPadOffset, 0, kPadSize);
        fKey.setKeyData(keyData.fKey32);
    }
    GrBinHashKey<kKeySize> fKey;
};


struct GrResourceInvalidatedMessage {
    GrResourceKey key;
};



class GrResourceCacheEntry {
public:
    GrGpuResource* resource() const { return fResource; }
    const GrResourceKey& key() const { return fKey; }

    static const GrResourceKey& GetKey(const GrResourceCacheEntry& e) { return e.key(); }
    static uint32_t Hash(const GrResourceKey& key) { return key.getHash(); }
#ifdef SK_DEBUG
    void validate() const;
#else
    void validate() const {}
#endif

    




    void didChangeResourceSize();

private:
    GrResourceCacheEntry(GrResourceCache* resourceCache,
                         const GrResourceKey& key,
                         GrGpuResource* resource);
    ~GrResourceCacheEntry();

    GrResourceCache* fResourceCache;
    GrResourceKey    fKey;
    GrGpuResource*   fResource;
    size_t           fCachedSize;
    bool             fIsExclusive;

    
    SK_DECLARE_INTERNAL_LLIST_INTERFACE(GrResourceCacheEntry);

    friend class GrResourceCache;
};




















class GrResourceCache {
public:
    GrResourceCache(int maxCount, size_t maxBytes);
    ~GrResourceCache();

    







    void getLimits(int* maxResources, size_t* maxBytes) const;

    








    void setLimits(int maxResources, size_t maxResourceBytes);

    





    typedef bool (*PFOverbudgetCB)(void* data);

    





    void setOverbudgetCallback(PFOverbudgetCB overbudgetCB, void* data) {
        fOverbudgetCB = overbudgetCB;
        fOverbudgetData = data;
    }

    


    size_t getCachedResourceBytes() const { return fEntryBytes; }

    


    int getCachedResourceCount() const { return fEntryCount; }

    
    
    enum OwnershipFlags {
        kNoOtherOwners_OwnershipFlag = 0x1, 
        kHide_OwnershipFlag = 0x2  
    };

    










    GrGpuResource* find(const GrResourceKey& key,
                        uint32_t ownershipFlags = 0);

    










    void addResource(const GrResourceKey& key,
                     GrGpuResource* resource,
                     uint32_t ownershipFlags = 0);

    



    bool hasKey(const GrResourceKey& key) const { return NULL != fCache.find(key); }

    





    void makeExclusive(GrResourceCacheEntry* entry);

    



    void makeNonExclusive(GrResourceCacheEntry* entry);

    


    void didIncreaseResourceSize(const GrResourceCacheEntry*, size_t amountInc);
    void didDecreaseResourceSize(const GrResourceCacheEntry*, size_t amountDec);

    


    void deleteResource(GrResourceCacheEntry* entry);

    


    void purgeAllUnlocked();

    










    void purgeAsNeeded(int extraCount = 0, size_t extraBytes = 0);

#ifdef SK_DEBUG
    void validate() const;
#else
    void validate() const {}
#endif

#if GR_CACHE_STATS
    void printStats();
#endif

private:
    enum BudgetBehaviors {
        kAccountFor_BudgetBehavior,
        kIgnore_BudgetBehavior
    };

    void internalDetach(GrResourceCacheEntry*, BudgetBehaviors behavior = kAccountFor_BudgetBehavior);
    void attachToHead(GrResourceCacheEntry*, BudgetBehaviors behavior = kAccountFor_BudgetBehavior);

    void removeInvalidResource(GrResourceCacheEntry* entry);

    SkTMultiMap<GrResourceCacheEntry, GrResourceKey> fCache;

    
    typedef SkTInternalLList<GrResourceCacheEntry> EntryList;
    EntryList      fList;

#ifdef SK_DEBUG
    
    EntryList      fExclusiveList;
#endif

    
    int            fMaxCount;
    size_t         fMaxBytes;

    
#if GR_CACHE_STATS
    int            fHighWaterEntryCount;
    size_t         fHighWaterEntryBytes;
    int            fHighWaterClientDetachedCount;
    size_t         fHighWaterClientDetachedBytes;
#endif

    int            fEntryCount;
    size_t         fEntryBytes;
    int            fClientDetachedCount;
    size_t         fClientDetachedBytes;

    
    bool           fPurging;

    PFOverbudgetCB fOverbudgetCB;
    void*          fOverbudgetData;

    void internalPurge(int extraCount, size_t extraBytes);

    
    SkMessageBus<GrResourceInvalidatedMessage>::Inbox fInvalidationInbox;
    void purgeInvalidated();

#ifdef SK_DEBUG
    static size_t countBytes(const SkTInternalLList<GrResourceCacheEntry>& list);
#endif
};



#ifdef SK_DEBUG
    class GrAutoResourceCacheValidate {
    public:
        GrAutoResourceCacheValidate(GrResourceCache* cache) : fCache(cache) {
            cache->validate();
        }
        ~GrAutoResourceCacheValidate() {
            fCache->validate();
        }
    private:
        GrResourceCache* fCache;
    };
#else
    class GrAutoResourceCacheValidate {
    public:
        GrAutoResourceCacheValidate(GrResourceCache*) {}
    };
#endif

#endif
