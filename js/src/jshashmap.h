






































#ifndef jshashmap_h_
#define jshashmap_h_

#include "jspool.h"
#include "jsbit.h"

namespace js {


template <class Key> struct DefaultHasher {
    static uint32 hash(const Key &key) {
        
        return key;
    }
};
template <class T> struct DefaultHasher<T *> {
    static uint32 hash(const T *key) {
        
        return (uint32)(unsigned long)key >> 2;
    }
};




















template <class Key, class Value, class Hasher,
          size_t N, class AllocPolicy>
class HashMap
{
  public:
    
    struct Element
    {
        const Key   key;
        Value       value;
    };

  private:
    
    void initTable();
    void destroyAll();

    typedef uint32 HashNumber;

    struct HashEntry : Element
    {
        HashEntry   *next;
        HashNumber  keyHash;
    };

    struct InternalRange;
    InternalRange internalAll() const;

    
    void *malloc(size_t bytes) { return entryPool.allocPolicy().malloc(bytes); }
    void free(void *p) { entryPool.allocPolicy().free(p); }
    void reportAllocOverflow() { entryPool.allocPolicy().reportAllocOverflow(); }

    
    static const size_t sHashBits = 32;

    

    



    static const size_t sInlineCount =
        tl::Max<2, tl::RoundUpPow2<(8 * N) / 7>::result>::result;

    static const size_t sInlineTableShift =
        sHashBits - tl::FloorLog2<sInlineCount>::result;

    static const size_t sTableElemBytes =
        sizeof(HashEntry *);

    static const size_t sInlineBytes =
        sInlineCount * sTableElemBytes;

    static const uint32 sGoldenRatio = 0x9E3779B9U; 

    static const size_t sMinHashShrinkLimit =
        tl::Max<64, sInlineCount>::result;

    

    
    size_t numElem;

    
    uint32 shift;

    
    char tableBuf[sInlineBytes];

    
    HashEntry **table;

    
    Pool<HashEntry, N, AllocPolicy> entryPool;

#ifdef DEBUG
    friend class ReentrancyGuard;
    bool entered;
#endif

    

    uint32 tableCapacity() const {
        JS_ASSERT(shift > 0 && shift <= sHashBits);
        return uint32(1) << (sHashBits - shift);
    }

    bool overloaded(size_t tableCapacity) const {
        
        return numElem >= (tableCapacity - (tableCapacity >> 3));
    }

    bool underloaded(size_t tableCapacity) const {
        
        return numElem < (tableCapacity >> 2) &&
               tableCapacity > sMinHashShrinkLimit;
    }

    HashEntry **hashToBucket(HashNumber hn) const {
        JS_ASSERT(shift > 0 && shift < sHashBits);
        return table + ((hn * sGoldenRatio) >> shift);
    }

  public:
    

    HashMap(AllocPolicy = AllocPolicy());
    ~HashMap();

    HashMap(const HashMap &);
    HashMap &operator=(const HashMap &);

    




    class Pointer {
        typedef void (Pointer::* ConvertibleToBool)();

        friend class HashMap;
        void nonNull() {}
        Pointer(HashEntry **e, HashNumber h) : hepp(e), keyHash(h) {}

        HashEntry **hepp;
        HashNumber keyHash;

      public:
        typedef Element ElementType;

        bool operator==(const Pointer &rhs) const  { return *hepp == *rhs.hepp; }
        bool operator!=(const Pointer &rhs) const  { return *hepp != *rhs.hepp; }

        bool null() const                          { return !*hepp; }
        operator ConvertibleToBool();

        
        ElementType &operator*() const                 { return **hepp; }
        ElementType *operator->() const                { return *hepp; }
    };

    
    class Range {
        friend class HashMap;
        Range(HashEntry *hep, HashEntry **te, HashEntry **end)
          : hep(hep), tableEntry(te), tableEnd(end) {}

        HashEntry *hep, **tableEntry, **tableEnd;

      public:
        typedef Element ElementType;

        
        bool empty() const              { return tableEntry == tableEnd; }
        ElementType &front() const      { return *hep; }
        void popFront();
    };

    
    Range all() const;
    bool empty() const { return numElem == 0; }
    size_t count() const { return numElem; }

    





    Pointer lookup(const Key &k) const;
    Pointer lookup(const Key &k, HashNumber keyHash) const;

    







    bool put(const Key &k, const Value &v);
    bool put(const Key &k, const Value &v, HashNumber keyHash);
    bool addAfterMiss(const Key &k, const Value &v, Pointer &ptr);

    





    Value *findOrAdd(const Key &k);

    




    void remove(const Key &k);
    void remove(const Key &k, HashNumber);
    void remove(Pointer);

    






    void clear(bool freeCache = true);
};



template <class K, class V, class H, size_t N, class AP>
inline void
HashMap<K,V,H,N,AP>::initTable()
{
    shift = sInlineTableShift;
    table = reinterpret_cast<HashEntry **>(tableBuf);
    for (HashEntry **p = table, **end = table + sInlineCount; p != end; ++p)
        *p = NULL;
}

template <class K, class V, class H, size_t N, class AP>
inline
HashMap<K,V,H,N,AP>::HashMap(AP ap)
  : numElem(0),
    entryPool(ap)
#ifdef DEBUG
    , entered(false)
#endif
{
    initTable();
}

template <class K, class V, class H, size_t N, class AP>
inline void
HashMap<K,V,H,N,AP>::destroyAll()
{
    
    if (!tl::IsPodType<K>::result || !tl::IsPodType<V>::result)
        entryPool.clear(internalAll());
    else
        entryPool.freeRawMemory();

    
    if ((void *)table != (void *)tableBuf)
        this->free(table);
}

template <class K, class V, class H, size_t N, class AP>
inline
HashMap<K,V,H,N,AP>::~HashMap()
{
    ReentrancyGuard g(*this);
    destroyAll();
}

template <class K, class V, class H, size_t N, class AP>
inline
HashMap<K,V,H,N,AP>::Pointer::operator ConvertibleToBool()
{
    return null() ? NULL : &Pointer::nonNull;
}

template <class K, class V, class H, size_t N, class AP>
inline typename HashMap<K,V,H,N,AP>::InternalRange
HashMap<K,V,H,N,AP>::internalAll() const
{
    HashEntry **p = table, **end = table + tableCapacity();
    while (p != end && !*p)
        ++p;

    
    return Range(p == end ? NULL : *p, p, end);
}

template <class K, class V, class H, size_t N, class AP>
inline typename HashMap<K,V,H,N,AP>::Range
HashMap<K,V,H,N,AP>::all() const
{
    JS_ASSERT(!entered);
    return internalAll();
}

template <class K, class V, class H, size_t N, class AP>
inline void
HashMap<K,V,H,N,AP>::Range::popFront()
{
    if ((hep = hep->next))
        return;
    ++tableEntry;
    for (; tableEntry != tableEnd; ++tableEntry) {
        if ((hep = *tableEntry))
            return;
    }
    hep = NULL;
}

template <class K, class V, class H, size_t N, class AP>
inline typename HashMap<K,V,H,N,AP>::Pointer
HashMap<K,V,H,N,AP>::lookup(const K &k, HashNumber hash) const
{
    JS_ASSERT(!entered);
    HashEntry **bucket = hashToBucket(hash), **p = bucket;

    
    for (HashEntry *e = *p; e; p = &e->next, e = *p) {
        if (e->keyHash == hash && k == e->key) {
            
            *p = e->next;
            e->next = *bucket;
            *bucket = e;
            return Pointer(bucket, hash);
        }
    }

    
    return Pointer(p, hash);
}

template <class K, class V, class H, size_t N, class AP>
inline typename HashMap<K,V,H,N,AP>::Pointer
HashMap<K,V,H,N,AP>::lookup(const K &k) const
{
    return lookup(k, H::hash(k));
}

template <class K, class V, class H, size_t N, class AP>
inline bool
HashMap<K,V,H,N,AP>::addAfterMiss(const K &k, const V &v, Pointer &ptr)
{
    ReentrancyGuard g(*this);
    JS_ASSERT(ptr.null());

    
    uint32 cap = tableCapacity();
    if (overloaded(cap)) {
        
        if (shift <= 1 + tl::CeilingLog2<sTableElemBytes>::result) {
            this->reportAllocOverflow();
            return false;
        }
        size_t bytes = cap * 2 * sTableElemBytes;

        
        HashEntry **newTable = (HashEntry **)this->malloc(bytes);
        if (!newTable)
            return false;
        memset(newTable, NULL, bytes);

        
        HashEntry **oldTable = table;
        table = newTable;
        --shift;
        JS_ASSERT(shift > 0);

        
        for (HashEntry **p = oldTable, **end = oldTable + cap; p != end; ++p) {
            for (HashEntry *e = *p, *next; e; e = next) {
                next = e->next;
                HashEntry **dstBucket = hashToBucket(e->keyHash);
                e->next = *dstBucket;
                *dstBucket = e;
            }
        }

        
        if ((void *)oldTable != (void *)tableBuf) {
            
            this->free(oldTable);
        } else {
            
            size_t usable = sInlineBytes - sInlineBytes % sizeof(HashEntry);
            entryPool.lendUnusedMemory(tableBuf, tableBuf + usable);
        }

        
        ptr.hepp = hashToBucket(ptr.keyHash);
    }

    
    HashEntry *alloc = entryPool.create();
    if (!alloc)
        return false;
    const_cast<K &>(alloc->key) = k;
    alloc->value = v;
    alloc->keyHash = ptr.keyHash;
    alloc->next = *ptr.hepp;  
    *ptr.hepp = alloc;
    ++numElem;

    return true;
}

template <class K, class V, class H, size_t N, class AP>
inline bool
HashMap<K,V,H,N,AP>::put(const K &k, const V &v, HashNumber hn)
{
    JS_ASSERT(!entered);
    Pointer p = lookup(k, hn);
    if (p.null())
        return addAfterMiss(k, v, p);
    p->value = v;
    return true;
}

template <class K, class V, class H, size_t N, class AP>
inline bool
HashMap<K,V,H,N,AP>::put(const K &k, const V &v)
{
    return put(k, v, H::hash(k));
}

template <class K, class V, class H, size_t N, class AP>
inline V *
HashMap<K,V,H,N,AP>::findOrAdd(const K &k)
{
    JS_ASSERT(!entered);
    Pointer p = lookup(k);
    if (p.null() && !addAfterMiss(k, V(), p))
        return NULL;
    JS_ASSERT(!p.null());
    return &p->value;
}





template <class K, class V, class H, size_t N, class AP>
struct HashMap<K,V,H,N,AP>::InternalRange : Range {
    InternalRange(const Range &r) : Range(r) {}
    HashEntry &front() { return static_cast<HashEntry &>(Range::front()); }
};

template <class K, class V, class H, size_t N, class AP>
inline void
HashMap<K,V,H,N,AP>::remove(Pointer p)
{
    ReentrancyGuard g(*this);

    
    HashEntry *e = *p.hepp;
    *p.hepp = e->next;
    --numElem;
    entryPool.destroy(e);
    size_t cap = tableCapacity();

    if (underloaded(cap)) {
        
        JS_ASSERT((void *)table != (void *)tableBuf);

        
        size_t newCap = cap >> 1;
        size_t tableBytes;
        HashEntry **newTable;
        if (newCap <= sInlineCount) {
            newCap = sInlineCount;
            tableBytes = sInlineBytes;
            newTable = (HashEntry **)tableBuf;
        } else {
            tableBytes = newCap * sTableElemBytes;
            newTable = (HashEntry **)this->malloc(tableBytes);
            if (!newTable)
                return;
        }

        
        InternalRange r = internalAll();
        HashEntry *array = static_cast<HashEntry *>(
                                entryPool.consolidate(r, numElem));
        if (!array) {
            if ((void *)newTable != (void *)tableBuf)
                this->free(newTable);
            return;
        }

        
        this->free(table);
        table = newTable;
        ++shift;
        memset(newTable, NULL, tableBytes);

        
        for (HashEntry *p = array, *end = array + numElem; p != end; ++p) {
            HashEntry **dstBucket = hashToBucket(p->keyHash);
            p->next = *dstBucket;
            *dstBucket = p;
        }
    }
}

template <class K, class V, class H, size_t N, class AP>
inline void
HashMap<K,V,H,N,AP>::remove(const K &k, HashNumber hash)
{
    JS_ASSERT(!entered);
    if (Pointer p = lookup(k, hash))
        remove(p);
}

template <class K, class V, class H, size_t N, class AP>
inline void
HashMap<K,V,H,N,AP>::remove(const K &k)
{
    return remove(k, H::hash(k));
}

template <class K, class V, class H, size_t N, class AP>
inline void
HashMap<K,V,H,N,AP>::clear(bool freeCache)
{
    ReentrancyGuard g(*this);
    destroyAll();
    numElem = 0;
    initTable();
}

}  

#endif
