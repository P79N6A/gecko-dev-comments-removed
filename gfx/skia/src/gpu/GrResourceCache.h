









#ifndef GrResourceCache_DEFINED
#define GrResourceCache_DEFINED

#include "GrConfig.h"
#include "GrTypes.h"
#include "GrTHashCache.h"
#include "SkTDLinkedList.h"

class GrResource;



#define RET_IF_LT_OR_GT(a, b)   \
    do {                        \
        if ((a) < (b)) {        \
            return true;        \
        }                       \
        if ((b) < (a)) {        \
            return false;       \
        }                       \
    } while (0)






class GrResourceKey {
public:
    enum {
        kHashBits   = 7,
        kHashCount  = 1 << kHashBits,
        kHashMask   = kHashCount - 1
    };

    GrResourceKey(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3) {
        fP[0] = p0;
        fP[1] = p1;
        fP[2] = p2;
        fP[3] = p3;
        this->computeHashIndex();
    }

    GrResourceKey(uint32_t v[4]) {
        memcpy(fP, v, 4 * sizeof(uint32_t));
        this->computeHashIndex();
    }

    GrResourceKey(const GrResourceKey& src) {
        memcpy(fP, src.fP, 4 * sizeof(uint32_t));
#if GR_DEBUG
        this->computeHashIndex();
        GrAssert(fHashIndex == src.fHashIndex);
#endif
        fHashIndex = src.fHashIndex;
    }

    
    int hashIndex() const { return fHashIndex; }

    friend bool operator==(const GrResourceKey& a, const GrResourceKey& b) {
        GR_DEBUGASSERT(-1 != a.fHashIndex && -1 != b.fHashIndex);
        return 0 == memcmp(a.fP, b.fP, 4 * sizeof(uint32_t));
    }

    friend bool operator!=(const GrResourceKey& a, const GrResourceKey& b) {
        GR_DEBUGASSERT(-1 != a.fHashIndex && -1 != b.fHashIndex);
        return !(a == b);
    }

    friend bool operator<(const GrResourceKey& a, const GrResourceKey& b) {
        RET_IF_LT_OR_GT(a.fP[0], b.fP[0]);
        RET_IF_LT_OR_GT(a.fP[1], b.fP[1]);
        RET_IF_LT_OR_GT(a.fP[2], b.fP[2]);
        return a.fP[3] < b.fP[3];
    }

    uint32_t getValue32(int i) const {
        GrAssert(i >=0 && i < 4);
        return fP[i];
    }
private:

    static uint32_t rol(uint32_t x) {
        return (x >> 24) | (x << 8);
    }
    static uint32_t ror(uint32_t x) {
        return (x >> 8) | (x << 24);
    }
    static uint32_t rohalf(uint32_t x) {
        return (x >> 16) | (x << 16);
    }

    void computeHashIndex() {
        uint32_t hash = fP[0] ^ rol(fP[1]) ^ ror(fP[2]) ^ rohalf(fP[3]);
        
        
        hash ^= hash >> 16;
        hash ^= hash >> 8;
        fHashIndex = hash & kHashMask;
    }

    uint32_t    fP[4];

    
    int         fHashIndex;

    friend class GrContext;
};


class GrCacheKey {
public:
    GrCacheKey(const GrTextureDesc& desc, const GrResourceKey& key)
        : fDesc(desc)
        , fKey(key) {
    }

    void set(const GrTextureDesc& desc, const GrResourceKey& key) {
        fDesc = desc;
        fKey = key;
    }

    const GrTextureDesc& desc() const { return fDesc; }
    const GrResourceKey& key() const { return fKey; }

protected:
    GrTextureDesc fDesc;
    GrResourceKey fKey;
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

    
    SK_DEFINE_DLINKEDLIST_INTERFACE(GrResourceEntry);

    friend class GrResourceCache;
    friend class GrDLinkedList;
};



#include "GrTHashCache.h"




















class GrResourceCache {
public:
    GrResourceCache(int maxCount, size_t maxBytes);
    ~GrResourceCache();

    







    void getLimits(int* maxResources, size_t* maxBytes) const;

    








    void setLimits(int maxResource, size_t maxResourceBytes);

    


    size_t getCachedResourceBytes() const { return fEntryBytes; }

    



    GrResource* find(const GrResourceKey& key);

    






    void create(const GrResourceKey&, GrResource*);

    



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
    void internalDetach(GrResourceEntry*, bool);
    void attachToHead(GrResourceEntry*, bool);

    void removeInvalidResource(GrResourceEntry* entry);

    class Key;
    GrTHashTable<GrResourceEntry, Key, 8> fCache;

    
    typedef SkTDLinkedList<GrResourceEntry> EntryList;
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
    static size_t countBytes(const SkTDLinkedList<GrResourceEntry>& list);
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
