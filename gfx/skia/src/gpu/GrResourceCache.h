









#ifndef GrResourceCache_DEFINED
#define GrResourceCache_DEFINED

#include "GrConfig.h"
#include "GrTypes.h"
#include "GrTHashCache.h"
#include "GrBinHashKey.h"
#include "SkTInternalLList.h"

class GrResource;
class GrResourceEntry;

class GrResourceKey {
public:
    enum {
        kHashBits   = 7,
        kHashCount  = 1 << kHashBits,
        kHashMask   = kHashCount - 1
    };

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
        fKey.fHashedKey.reset();
    }

    void reset(const GrCacheID& id, ResourceType type, ResourceFlags flags) {
        this->init(id.getDomain(), id.getKey(), type, flags);
    }

    
    int getHash() const {
        return fKey.fHashedKey.getHash() & kHashMask;
    }

    bool isScratch() const {
        return ScratchDomain() ==
            *reinterpret_cast<const GrCacheID::Domain*>(fKey.fHashedKey.getData() +
                                                        kCacheIDDomainOffset);
    }

    ResourceType getResourceType() const {
        return *reinterpret_cast<const ResourceType*>(fKey.fHashedKey.getData() +
                                                      kResourceTypeOffset);
    }

    ResourceFlags getResourceFlags() const {
        return *reinterpret_cast<const ResourceFlags*>(fKey.fHashedKey.getData() +
                                                       kResourceFlagsOffset);
    }

    int compare(const GrResourceKey& other) const {
        return fKey.fHashedKey.compare(other.fKey.fHashedKey);
    }

    static bool LT(const GrResourceKey& a, const GrResourceKey& b) {
        return a.compare(b) < 0;
    }

    static bool EQ(const GrResourceKey& a, const GrResourceKey& b) {
        return 0 == a.compare(b);
    }

    inline static bool LT(const GrResourceEntry& entry, const GrResourceKey& key);
    inline static bool EQ(const GrResourceEntry& entry, const GrResourceKey& key);
    inline static bool LT(const GrResourceEntry& a, const GrResourceEntry& b);
    inline static bool EQ(const GrResourceEntry& a, const GrResourceEntry& b);

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
        fKey.fHashedKey.setKeyData(keyData.fKey32);
    }

    struct Key;
    typedef GrTBinHashKey<Key, kKeySize> HashedKey;

    struct Key {
        int compare(const HashedKey& hashedKey) const {
            return fHashedKey.compare(hashedKey);
        }

        HashedKey fHashedKey;
    };

    Key fKey;
};



class GrResourceEntry {
public:
    GrResource* resource() const { return fResource; }
    const GrResourceKey& key() const { return fKey; }

#if GR_DEBUG
    void validate() const;
#else
    void validate() const {}
#endif

private:
    GrResourceEntry(const GrResourceKey& key, GrResource* resource);
    ~GrResourceEntry();

    GrResourceKey    fKey;
    GrResource*      fResource;

    
    SK_DECLARE_INTERNAL_LLIST_INTERFACE(GrResourceEntry);

    friend class GrResourceCache;
    friend class GrDLinkedList;
};

bool GrResourceKey::LT(const GrResourceEntry& entry, const GrResourceKey& key) {
    return LT(entry.key(), key);
}

bool GrResourceKey::EQ(const GrResourceEntry& entry, const GrResourceKey& key) {
    return EQ(entry.key(), key);
}

bool GrResourceKey::LT(const GrResourceEntry& a, const GrResourceEntry& b) {
    return LT(a.key(), b.key());
}

bool GrResourceKey::EQ(const GrResourceEntry& a, const GrResourceEntry& b) {
    return EQ(a.key(), b.key());
}



#include "GrTHashCache.h"




















class GrResourceCache {
public:
    GrResourceCache(int maxCount, size_t maxBytes);
    ~GrResourceCache();

    







    void getLimits(int* maxResources, size_t* maxBytes) const;

    








    void setLimits(int maxResource, size_t maxResourceBytes);

    


    size_t getCachedResourceBytes() const { return fEntryBytes; }

    
    
    enum OwnershipFlags {
        kNoOtherOwners_OwnershipFlag = 0x1, 
        kHide_OwnershipFlag = 0x2  
    };

    










    GrResource* find(const GrResourceKey& key,
                     uint32_t ownershipFlags = 0);

    










    void addResource(const GrResourceKey& key,
                     GrResource* resource,
                     uint32_t ownershipFlags = 0);

    



    bool hasKey(const GrResourceKey& key) const;

    





    void makeExclusive(GrResourceEntry* entry);

    



    void makeNonExclusive(GrResourceEntry* entry);

    


    void purgeAllUnlocked();

    




    void purgeAsNeeded();

#if GR_DEBUG
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

    void internalDetach(GrResourceEntry*, BudgetBehaviors behavior = kAccountFor_BudgetBehavior);
    void attachToHead(GrResourceEntry*, BudgetBehaviors behavior = kAccountFor_BudgetBehavior);

    void removeInvalidResource(GrResourceEntry* entry);

    GrTHashTable<GrResourceEntry, GrResourceKey, 8> fCache;

    
    typedef SkTInternalLList<GrResourceEntry> EntryList;
    EntryList    fList;

#if GR_DEBUG
    
    EntryList    fExclusiveList;
#endif

    
    int fMaxCount;
    size_t fMaxBytes;

    
#if GR_CACHE_STATS
    int fHighWaterEntryCount;
    size_t fHighWaterEntryBytes;
    int fHighWaterClientDetachedCount;
    size_t fHighWaterClientDetachedBytes;
#endif

    int fEntryCount;
    size_t fEntryBytes;
    int fClientDetachedCount;
    size_t fClientDetachedBytes;

    
    bool fPurging;

#if GR_DEBUG
    static size_t countBytes(const SkTInternalLList<GrResourceEntry>& list);
#endif
};



#if GR_DEBUG
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
