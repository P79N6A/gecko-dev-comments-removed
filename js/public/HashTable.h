






#ifndef jshashtable_h_
#define jshashtable_h_

#include "TemplateLib.h"
#include "Utility.h"

namespace js {

class TempAllocPolicy;



namespace detail {

template <class T, class HashPolicy, class AllocPolicy>
class HashTable;

template <class T>
class HashTableEntry {
    HashNumber keyHash;

    typedef typename tl::StripConst<T>::result NonConstT;

    static const HashNumber sFreeKey = 0;
    static const HashNumber sRemovedKey = 1;
    static const HashNumber sCollisionBit = 1;

    template <class, class, class> friend class HashTable;

    static bool isLiveHash(HashNumber hash)
    {
        return hash > sRemovedKey;
    }

  public:
    HashTableEntry() : keyHash(0), t() {}
    HashTableEntry(MoveRef<HashTableEntry> rhs) : keyHash(rhs->keyHash), t(Move(rhs->t)) { }
    void operator=(const HashTableEntry &rhs) { keyHash = rhs.keyHash; t = rhs.t; }
    void operator=(MoveRef<HashTableEntry> rhs) { keyHash = rhs->keyHash; t = Move(rhs->t); }

    NonConstT t;

    bool isFree() const           { return keyHash == sFreeKey; }
    void setFree()                { keyHash = sFreeKey; t = T(); }
    bool isRemoved() const        { return keyHash == sRemovedKey; }
    void setRemoved()             { keyHash = sRemovedKey; t = T(); }
    bool isLive() const           { return isLiveHash(keyHash); }
    void setLive(HashNumber hn)   { JS_ASSERT(isLiveHash(hn)); keyHash = hn; }

    void setCollision()           { JS_ASSERT(isLive()); keyHash |= sCollisionBit; }
    void setCollision(HashNumber collisionBit) {
        JS_ASSERT(isLive()); keyHash |= collisionBit;
    }
    void unsetCollision()         { keyHash &= ~sCollisionBit; }
    bool hasCollision() const     { return keyHash & sCollisionBit; }
    bool matchHash(HashNumber hn) { return (keyHash & ~sCollisionBit) == hn; }
    HashNumber getKeyHash() const { JS_ASSERT(!hasCollision()); return keyHash; }
};








template <class T, class HashPolicy, class AllocPolicy>
class HashTable : private AllocPolicy
{
    typedef typename tl::StripConst<T>::result NonConstT;
    typedef typename HashPolicy::KeyType Key;
    typedef typename HashPolicy::Lookup Lookup;

  public:
    typedef HashTableEntry<T> Entry;

    





    class Ptr
    {
        friend class HashTable;
        typedef void (Ptr::* ConvertibleToBool)();
        void nonNull() {}

        Entry *entry;

      protected:
        Ptr(Entry &entry) : entry(&entry) {}

      public:
        
        Ptr() {
#ifdef DEBUG
            entry = (Entry *)0xbad;
#endif
        }

        bool found() const                    { return entry->isLive(); }
        operator ConvertibleToBool() const    { return found() ? &Ptr::nonNull : 0; }
        bool operator==(const Ptr &rhs) const { JS_ASSERT(found() && rhs.found()); return entry == rhs.entry; }
        bool operator!=(const Ptr &rhs) const { return !(*this == rhs); }

        T &operator*() const                  { return entry->t; }
        T *operator->() const                 { return &entry->t; }
    };

    
    class AddPtr : public Ptr
    {
        friend class HashTable;
        HashNumber keyHash;
        DebugOnly<uint64_t> mutationCount;

        AddPtr(Entry &entry, HashNumber hn) : Ptr(entry), keyHash(hn) {}
      public:
        
        AddPtr() {}
    };

    





    class Range
    {
      protected:
        friend class HashTable;

        Range(Entry *c, Entry *e) : cur(c), end(e), validEntry(true) {
            while (cur < end && !cur->isLive())
                ++cur;
        }

        Entry *cur, *end;
        DebugOnly<bool> validEntry;

      public:
        Range() : cur(NULL), end(NULL), validEntry(false) {}

        bool empty() const {
            return cur == end;
        }

        T &front() const {
            JS_ASSERT(validEntry);
            JS_ASSERT(!empty());
            return cur->t;
        }

        void popFront() {
            JS_ASSERT(!empty());
            while (++cur < end && !cur->isLive())
                continue;
            validEntry = true;
        }
    };

    








    class Enum : public Range
    {
        friend class HashTable;

        HashTable &table;
        bool rekeyed;
        bool removed;

        
        Enum(const Enum &);
        void operator=(const Enum &);

      public:
        template<class Map> explicit
        Enum(Map &map) : Range(map.all()), table(map.impl), rekeyed(false), removed(false) {}

        








        void removeFront() {
            table.remove(*this->cur);
            removed = true;
            this->validEntry = false;
        }

        




        void rekeyFront(const Lookup &l, const Key &k) {
            typename HashTableEntry<T>::NonConstT t = this->cur->t;
            HashPolicy::setKey(t, const_cast<Key &>(k));
            table.remove(*this->cur);
            table.putNewInfallible(l, t);
            rekeyed = true;
            this->validEntry = false;
        }

        void rekeyFront(const Key &k) {
            rekeyFront(k, k);
        }

        
        ~Enum() {
            if (rekeyed)
                table.checkOverRemoved();
            if (removed)
                table.checkUnderloaded();
        }
    };

  private:
    uint32_t    hashShift;      
    uint32_t    entryCount;     
    uint32_t    gen;            
    uint32_t    removedCount;   
    Entry       *table;         

    void setTableSizeLog2(unsigned sizeLog2) {
        hashShift = sHashBits - sizeLog2;
    }

#ifdef DEBUG
    mutable struct Stats {
        uint32_t        searches;       
        uint32_t        steps;          
        uint32_t        hits;           
        uint32_t        misses;         
        uint32_t        addOverRemoved; 
        uint32_t        removes;        
        uint32_t        removeFrees;    
        uint32_t        grows;          
        uint32_t        shrinks;        
        uint32_t        compresses;     
        uint32_t        rehashes;       
    } stats;
#   define METER(x) x
#else
#   define METER(x)
#endif

    friend class js::ReentrancyGuard;
    mutable DebugOnly<bool> entered;
    DebugOnly<uint64_t>     mutationCount;

    
    static const unsigned sMinSizeLog2  = 2;
    static const unsigned sMinSize      = 1 << sMinSizeLog2;
    static const unsigned sDefaultInitSizeLog2 = 4;
  public:
    static const unsigned sDefaultInitSize = 1 << sDefaultInitSizeLog2;
  private:
    static const unsigned sMaxInit      = JS_BIT(23);
    static const unsigned sMaxCapacity  = JS_BIT(24);
    static const unsigned sHashBits     = tl::BitSize<HashNumber>::result;
    static const uint8_t  sMinAlphaFrac = 64;  
    static const uint8_t  sMaxAlphaFrac = 192; 
    static const uint8_t  sInvMaxAlpha  = 171; 
    static const HashNumber sFreeKey = Entry::sFreeKey;
    static const HashNumber sRemovedKey = Entry::sRemovedKey;
    static const HashNumber sCollisionBit = Entry::sCollisionBit;

    static void staticAsserts()
    {
        
        JS_STATIC_ASSERT(((sMaxInit * sInvMaxAlpha) >> 7) < sMaxCapacity);
        JS_STATIC_ASSERT((sMaxCapacity * sInvMaxAlpha) <= UINT32_MAX);
        JS_STATIC_ASSERT((sMaxCapacity * sizeof(Entry)) <= UINT32_MAX);
    }

    static bool isLiveHash(HashNumber hash)
    {
        return Entry::isLiveHash(hash);
    }

    static HashNumber prepareHash(const Lookup& l)
    {
        HashNumber keyHash = ScrambleHashCode(HashPolicy::hash(l));

        
        if (!isLiveHash(keyHash))
            keyHash -= (sRemovedKey + 1);
        return keyHash & ~sCollisionBit;
    }

    static Entry *createTable(AllocPolicy &alloc, uint32_t capacity)
    {
        Entry *newTable = (Entry *)alloc.malloc_(capacity * sizeof(Entry));
        if (!newTable)
            return NULL;
        for (Entry *e = newTable, *end = e + capacity; e < end; ++e)
            new(e) Entry();
        return newTable;
    }

    static void destroyTable(AllocPolicy &alloc, Entry *oldTable, uint32_t capacity)
    {
        for (Entry *e = oldTable, *end = e + capacity; e < end; ++e)
            e->~Entry();
        alloc.free_(oldTable);
    }

  public:
    HashTable(AllocPolicy ap)
      : AllocPolicy(ap),
        hashShift(sHashBits),
        entryCount(0),
        gen(0),
        removedCount(0),
        table(NULL),
        entered(false),
        mutationCount(0)
    {}

    MOZ_WARN_UNUSED_RESULT bool init(uint32_t length)
    {
        
        JS_ASSERT(table == NULL);

        



        if (length > sMaxInit) {
            this->reportAllocOverflow();
            return false;
        }
        uint32_t capacity = (length * sInvMaxAlpha) >> 7;

        if (capacity < sMinSize)
            capacity = sMinSize;

        
        uint32_t roundUp = sMinSize, roundUpLog2 = sMinSizeLog2;
        while (roundUp < capacity) {
            roundUp <<= 1;
            ++roundUpLog2;
        }

        capacity = roundUp;
        JS_ASSERT(capacity <= sMaxCapacity);

        table = createTable(*this, capacity);
        if (!table)
            return false;

        setTableSizeLog2(roundUpLog2);
        METER(memset(&stats, 0, sizeof(stats)));
        return true;
    }

    bool initialized() const
    {
        return !!table;
    }

    ~HashTable()
    {
        if (table)
            destroyTable(*this, table, capacity());
    }

  private:
    static HashNumber hash1(HashNumber hash0, uint32_t shift) {
        return hash0 >> shift;
    }

    struct DoubleHash {
        HashNumber h2;
        HashNumber sizeMask;
    };

    DoubleHash hash2(HashNumber curKeyHash, uint32_t hashShift) const {
        unsigned sizeLog2 = sHashBits - hashShift;
        DoubleHash dh = {
            ((curKeyHash << sizeLog2) >> hashShift) | 1,
            (HashNumber(1) << sizeLog2) - 1
        };
        return dh;
    }

    static HashNumber applyDoubleHash(HashNumber h1, const DoubleHash &dh) {
        return (h1 - dh.h2) & dh.sizeMask;
    }

    bool overloaded() {
        return entryCount + removedCount >= ((sMaxAlphaFrac * capacity()) >> 8);
    }

    bool underloaded() {
        uint32_t tableCapacity = capacity();
        return tableCapacity > sMinSize &&
               entryCount <= ((sMinAlphaFrac * tableCapacity) >> 8);
    }

    static bool match(Entry &e, const Lookup &l) {
        return HashPolicy::match(HashPolicy::getKey(e.t), l);
    }

    Entry &lookup(const Lookup &l, HashNumber keyHash, unsigned collisionBit) const
    {
        JS_ASSERT(isLiveHash(keyHash));
        JS_ASSERT(!(keyHash & sCollisionBit));
        JS_ASSERT(collisionBit == 0 || collisionBit == sCollisionBit);
        JS_ASSERT(table);
        METER(stats.searches++);

        
        HashNumber h1 = hash1(keyHash, hashShift);
        Entry *entry = &table[h1];

        
        if (entry->isFree()) {
            METER(stats.misses++);
            return *entry;
        }

        
        if (entry->matchHash(keyHash) && match(*entry, l)) {
            METER(stats.hits++);
            return *entry;
        }

        
        DoubleHash dh = hash2(keyHash, hashShift);

        
        Entry *firstRemoved = NULL;

        while(true) {
            if (JS_UNLIKELY(entry->isRemoved())) {
                if (!firstRemoved)
                    firstRemoved = entry;
            } else {
                entry->setCollision(collisionBit);
            }

            METER(stats.steps++);
            h1 = applyDoubleHash(h1, dh);

            entry = &table[h1];
            if (entry->isFree()) {
                METER(stats.misses++);
                return firstRemoved ? *firstRemoved : *entry;
            }

            if (entry->matchHash(keyHash) && match(*entry, l)) {
                METER(stats.hits++);
                return *entry;
            }
        }
    }

    







    Entry &findFreeEntry(HashNumber keyHash)
    {
        JS_ASSERT(!(keyHash & sCollisionBit));
        JS_ASSERT(table);
        METER(stats.searches++);

        

        
        HashNumber h1 = hash1(keyHash, hashShift);
        Entry *entry = &table[h1];

        
        if (!entry->isLive()) {
            METER(stats.misses++);
            return *entry;
        }

        
        DoubleHash dh = hash2(keyHash, hashShift);

        while(true) {
            JS_ASSERT(!entry->isRemoved());
            entry->setCollision();

            METER(stats.steps++);
            h1 = applyDoubleHash(h1, dh);

            entry = &table[h1];
            if (!entry->isLive()) {
                METER(stats.misses++);
                return *entry;
            }
        }
    }

    enum RebuildStatus { NotOverloaded, Rehashed, RehashFailed };

    RebuildStatus changeTableSize(int deltaLog2)
    {
        
        Entry *oldTable = table;
        uint32_t oldCap = capacity();
        uint32_t newLog2 = sHashBits - hashShift + deltaLog2;
        uint32_t newCapacity = JS_BIT(newLog2);
        if (newCapacity > sMaxCapacity) {
            this->reportAllocOverflow();
            return RehashFailed;
        }

        Entry *newTable = createTable(*this, newCapacity);
        if (!newTable)
            return RehashFailed;

        
        setTableSizeLog2(newLog2);
        removedCount = 0;
        gen++;
        table = newTable;

        
        for (Entry *src = oldTable, *end = src + oldCap; src < end; ++src) {
            if (src->isLive()) {
                src->unsetCollision();
                findFreeEntry(src->getKeyHash()) = Move(*src);
            }
        }

        destroyTable(*this, oldTable, oldCap);
        return Rehashed;
    }

    RebuildStatus checkOverloaded()
    {
        if (!overloaded())
            return NotOverloaded;

        
        int deltaLog2;
        if (removedCount >= (capacity() >> 2)) {
            METER(stats.compresses++);
            deltaLog2 = 0;
        } else {
            METER(stats.grows++);
            deltaLog2 = 1;
        }

        return changeTableSize(deltaLog2);
    }

    
    void checkOverRemoved()
    {
        if (overloaded()) {
            METER(stats.rehashes++);
            rehashTable();
            JS_ASSERT(!overloaded());
        }
    }

    void remove(Entry &e)
    {
        JS_ASSERT(table);
        METER(stats.removes++);

        if (e.hasCollision()) {
            e.setRemoved();
            removedCount++;
        } else {
            METER(stats.removeFrees++);
            e.setFree();
        }
        entryCount--;
        mutationCount++;
    }

    void checkUnderloaded()
    {
        if (underloaded()) {
            METER(stats.shrinks++);
            (void) changeTableSize(-1);
        }
    }

    






    void rehashTable()
    {
        removedCount = 0;
        for (size_t i = 0; i < capacity(); ++i)
            table[i].unsetCollision();

        for (size_t i = 0; i < capacity();) {
            Entry *src = &table[i];

            if (!src->isLive() || src->hasCollision()) {
                ++i;
                continue;
            }

            HashNumber keyHash = src->getKeyHash();
            HashNumber h1 = hash1(keyHash, hashShift);
            DoubleHash dh = hash2(keyHash, hashShift);
            Entry *tgt = &table[h1];
            while (true) {
                if (!tgt->hasCollision()) {
                    Swap(*src, *tgt);
                    tgt->setCollision();
                    break;
                }

                h1 = applyDoubleHash(h1, dh);
                tgt = &table[h1];
            }
        }

        






    }

  public:
    void clear()
    {
        if (tl::IsPodType<Entry>::result) {
            memset(table, 0, sizeof(*table) * capacity());
        } else {
            uint32_t tableCapacity = capacity();
            for (Entry *e = table, *end = table + tableCapacity; e < end; ++e)
                *e = Move(Entry());
        }
        removedCount = 0;
        entryCount = 0;
        mutationCount++;
    }

    void finish()
    {
        JS_ASSERT(!entered);

        if (!table)
            return;

        destroyTable(*this, table, capacity());
        table = NULL;
        gen++;
        entryCount = 0;
        removedCount = 0;
        mutationCount++;
    }

    Range all() const {
        JS_ASSERT(table);
        return Range(table, table + capacity());
    }

    bool empty() const {
        JS_ASSERT(table);
        return !entryCount;
    }

    uint32_t count() const {
        JS_ASSERT(table);
        return entryCount;
    }

    uint32_t capacity() const {
        JS_ASSERT(table);
        return JS_BIT(sHashBits - hashShift);
    }

    uint32_t generation() const {
        JS_ASSERT(table);
        return gen;
    }

    size_t sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf) const {
        return mallocSizeOf(table);
    }

    size_t sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf) const {
        return mallocSizeOf(this) + sizeOfExcludingThis(mallocSizeOf);
    }

    Ptr lookup(const Lookup &l) const {
        ReentrancyGuard g(*this);
        HashNumber keyHash = prepareHash(l);
        return Ptr(lookup(l, keyHash, 0));
    }

    AddPtr lookupForAdd(const Lookup &l) const {
        ReentrancyGuard g(*this);
        HashNumber keyHash = prepareHash(l);
        Entry &entry = lookup(l, keyHash, sCollisionBit);
        AddPtr p(entry, keyHash);
        p.mutationCount = mutationCount;
        return p;
    }

    bool add(AddPtr &p)
    {
        ReentrancyGuard g(*this);
        JS_ASSERT(mutationCount == p.mutationCount);
        JS_ASSERT(table);
        JS_ASSERT(!p.found());
        JS_ASSERT(!(p.keyHash & sCollisionBit));

        



        if (p.entry->isRemoved()) {
            METER(stats.addOverRemoved++);
            removedCount--;
            p.keyHash |= sCollisionBit;
        } else {
            
            RebuildStatus status = checkOverloaded();
            if (status == RehashFailed)
                return false;
            if (status == Rehashed)
                p.entry = &findFreeEntry(p.keyHash);
        }

        p.entry->setLive(p.keyHash);
        entryCount++;
        mutationCount++;
        return true;
    }

    




    bool add(AddPtr &p, T** pentry)
    {
        if (!add(p))
            return false;
        *pentry = &p.entry->t;
        return true;
    }

    bool add(AddPtr &p, const T &t)
    {
        if (!add(p))
            return false;
        p.entry->t = t;
        return true;
    }

    void putNewInfallible(const Lookup &l, const T &t)
    {
        JS_ASSERT(table);

        HashNumber keyHash = prepareHash(l);
        Entry *entry = &findFreeEntry(keyHash);

        if (entry->isRemoved()) {
            METER(stats.addOverRemoved++);
            removedCount--;
            keyHash |= sCollisionBit;
        }

        entry->t = t;
        entry->setLive(keyHash);
        entryCount++;
        mutationCount++;
    }

    bool putNew(const Lookup &l, const T &t)
    {
        if (checkOverloaded() == RehashFailed)
            return false;

        putNewInfallible(l, t);
        return true;
    }

    bool relookupOrAdd(AddPtr& p, const Lookup &l, const T& t)
    {
        p.mutationCount = mutationCount;
        {
            ReentrancyGuard g(*this);
            p.entry = &lookup(l, p.keyHash, sCollisionBit);
        }
        return p.found() || add(p, t);
    }

    void remove(Ptr p)
    {
        JS_ASSERT(table);
        ReentrancyGuard g(*this);
        JS_ASSERT(p.found());
        remove(*p.entry);
        checkUnderloaded();
    }

#undef METER
};

}  































template <class Key>
struct DefaultHasher
{
    typedef Key Lookup;
    static HashNumber hash(const Lookup &l) {
        
        return l;
    }
    static bool match(const Key &k, const Lookup &l) {
        
        return k == l;
    }
};





template <typename Key, size_t zeroBits>
struct PointerHasher
{
    typedef Key Lookup;
    static HashNumber hash(const Lookup &l) {
        size_t word = reinterpret_cast<size_t>(l) >> zeroBits;
        JS_STATIC_ASSERT(sizeof(HashNumber) == 4);
#if JS_BYTES_PER_WORD == 4
        return HashNumber(word);
#else
        JS_STATIC_ASSERT(sizeof word == 8);
        return HashNumber((word >> 32) ^ word);
#endif
    }
    static bool match(const Key &k, const Lookup &l) {
        return k == l;
    }
};

template <typename Key, size_t zeroBits>
struct TaggedPointerHasher
{
    typedef Key Lookup;

    static HashNumber hash(const Lookup &l) {
        return PointerHasher<Key, zeroBits>::hash(l);
    }

    static const uintptr_t COMPARE_MASK = uintptr_t(-1) - 1;

    static bool match(const Key &k, const Lookup &l) {
        return (uintptr_t(k) & COMPARE_MASK) == uintptr_t(l);
    }
};





template <class T>
struct DefaultHasher<T *>: PointerHasher<T *, tl::FloorLog2<sizeof(void *)>::result> { };



template <class Key, class Value>
class HashMapEntry
{
    template <class, class, class> friend class detail::HashTable;
    template <class> friend class detail::HashTableEntry;
    void operator=(const HashMapEntry &rhs) {
        const_cast<Key &>(key) = rhs.key;
        value = rhs.value;
    }

  public:
    HashMapEntry() : key(), value() {}

    template<typename KeyInput, typename ValueInput>
    HashMapEntry(const KeyInput &k, const ValueInput &v) : key(k), value(v) {}

    HashMapEntry(MoveRef<HashMapEntry> rhs)
      : key(Move(rhs->key)), value(Move(rhs->value)) { }
    void operator=(MoveRef<HashMapEntry> rhs) {
        const_cast<Key &>(key) = Move(rhs->key);
        value = Move(rhs->value);
    }

    const Key key;
    Value value;
};

namespace tl {

template <class T>
struct IsPodType<detail::HashTableEntry<T> > {
    static const bool result = IsPodType<T>::result;
};

template <class K, class V>
struct IsPodType<HashMapEntry<K, V> >
{
    static const bool result = IsPodType<K>::result && IsPodType<V>::result;
};

} 

















template <class Key,
          class Value,
          class HashPolicy = DefaultHasher<Key>,
          class AllocPolicy = TempAllocPolicy>
class HashMap
{
    typedef typename tl::StaticAssert<tl::IsRelocatableHeapType<Key>::result>::result keyAssert;
    typedef typename tl::StaticAssert<tl::IsRelocatableHeapType<Value>::result>::result valAssert;

  public:
    typedef typename HashPolicy::Lookup Lookup;

    typedef HashMapEntry<Key, Value> Entry;

  private:
    
    struct MapHashPolicy : HashPolicy
    {
        typedef Key KeyType;
        static const Key &getKey(Entry &e) { return e.key; }
        static void setKey(Entry &e, Key &k) { const_cast<Key &>(e.key) = k; }
    };
    typedef detail::HashTable<Entry, MapHashPolicy, AllocPolicy> Impl;

    friend class Impl::Enum;

    
    HashMap(const HashMap &);
    HashMap &operator=(const HashMap &);

    Impl impl;

  public:
    const static unsigned sDefaultInitSize = Impl::sDefaultInitSize;

    



    HashMap(AllocPolicy a = AllocPolicy()) : impl(a)  {}
    bool init(uint32_t len = sDefaultInitSize)        { return impl.init(len); }
    bool initialized() const                          { return impl.initialized(); }

    












    typedef typename Impl::Ptr Ptr;
    Ptr lookup(const Lookup &l) const                 { return impl.lookup(l); }

    
    void remove(Ptr p)                                { impl.remove(p); }

    


































    typedef typename Impl::AddPtr AddPtr;
    AddPtr lookupForAdd(const Lookup &l) const {
        return impl.lookupForAdd(l);
    }

    template<typename KeyInput, typename ValueInput>
    bool add(AddPtr &p, const KeyInput &k, const ValueInput &v) {
        Entry *pentry;
        if (!impl.add(p, &pentry))
            return false;
        const_cast<Key &>(pentry->key) = k;
        pentry->value = v;
        return true;
    }

    bool add(AddPtr &p, const Key &k, MoveRef<Value> v) {
        Entry *pentry;
        if (!impl.add(p, &pentry))
            return false;
        const_cast<Key &>(pentry->key) = k;
        pentry->value = v;
        return true;
    }

    bool add(AddPtr &p, const Key &k) {
        Entry *pentry;
        if (!impl.add(p, &pentry))
            return false;
        const_cast<Key &>(pentry->key) = k;
        return true;
    }

    template<typename KeyInput, typename ValueInput>
    bool relookupOrAdd(AddPtr &p, const KeyInput &k, const ValueInput &v) {
        return impl.relookupOrAdd(p, k, Entry(k, v));
    }

    









    typedef typename Impl::Range Range;
    Range all() const                                 { return impl.all(); }
    uint32_t count() const                            { return impl.count(); }
    size_t capacity() const                           { return impl.capacity(); }
    size_t sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf) const {
        return impl.sizeOfExcludingThis(mallocSizeOf);
    }
    size_t sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf) const {
        



        return mallocSizeOf(this) + impl.sizeOfExcludingThis(mallocSizeOf);
    }

    












    typedef typename Impl::Enum Enum;

    



    void clear()                                      { impl.clear(); }

    



    void finish()                                     { impl.finish(); }

   
    bool empty() const                                { return impl.empty(); }

    



    unsigned generation() const                       { return impl.generation(); }

    

    bool has(const Lookup &l) const {
        return impl.lookup(l) != NULL;
    }

    
    template<typename KeyInput, typename ValueInput>
    bool put(const KeyInput &k, const ValueInput &v) {
        AddPtr p = lookupForAdd(k);
        if (p) {
            p->value = v;
            return true;
        }
        return add(p, k, v);
    }

    
    bool putNew(const Key &k, const Value &v) {
        return impl.putNew(k, Entry(k, v));
    }

    
    Ptr lookupWithDefault(const Key &k, const Value &defaultValue) {
        AddPtr p = lookupForAdd(k);
        if (p)
            return p;
        (void)add(p, k, defaultValue);  
        return p;
    }

    
    void remove(const Lookup &l) {
        if (Ptr p = lookup(l))
            remove(p);
    }
};

















template <class T, class HashPolicy = DefaultHasher<T>, class AllocPolicy = TempAllocPolicy>
class HashSet
{
    typedef typename HashPolicy::Lookup Lookup;

    
    struct SetOps : HashPolicy {
        typedef T KeyType;
        static const KeyType &getKey(const T &t) { return t; }
        static void setKey(T &t, KeyType &k) { t = k; }
    };
    typedef detail::HashTable<const T, SetOps, AllocPolicy> Impl;

    friend class Impl::Enum;

    
    HashSet(const HashSet &);
    HashSet &operator=(const HashSet &);

    Impl impl;

  public:
    const static unsigned sDefaultInitSize = Impl::sDefaultInitSize;

    



    HashSet(AllocPolicy a = AllocPolicy()) : impl(a)  {}
    bool init(uint32_t len = sDefaultInitSize)        { return impl.init(len); }
    bool initialized() const                          { return impl.initialized(); }

    










    typedef typename Impl::Ptr Ptr;
    Ptr lookup(const Lookup &l) const                 { return impl.lookup(l); }

    
    void remove(Ptr p)                                { impl.remove(p); }

    

































    typedef typename Impl::AddPtr AddPtr;
    AddPtr lookupForAdd(const Lookup &l) const {
        return impl.lookupForAdd(l);
    }

    bool add(AddPtr &p, const T &t) {
        return impl.add(p, t);
    }

    bool relookupOrAdd(AddPtr &p, const Lookup &l, const T &t) {
        return impl.relookupOrAdd(p, l, t);
    }

    









    typedef typename Impl::Range Range;
    Range all() const                                 { return impl.all(); }
    uint32_t count() const                            { return impl.count(); }
    size_t capacity() const                           { return impl.capacity(); }
    size_t sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf) const {
        return impl.sizeOfExcludingThis(mallocSizeOf);
    }
    size_t sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf) const {
        



        return mallocSizeOf(this) + impl.sizeOfExcludingThis(mallocSizeOf);
    }

    












    typedef typename Impl::Enum Enum;

    



    void clear()                                      { impl.clear(); }

    



    void finish()                                     { impl.finish(); }

    
    bool empty() const                                { return impl.empty(); }

    



    unsigned generation() const                       { return impl.generation(); }

    

    bool has(const Lookup &l) const {
        return impl.lookup(l) != NULL;
    }

    
    bool put(const T &t) {
        AddPtr p = lookupForAdd(t);
        return p ? true : add(p, t);
    }

    
    bool putNew(const T &t) {
        return impl.putNew(t, t);
    }

    bool putNew(const Lookup &l, const T &t) {
        return impl.putNew(l, t);
    }

    void remove(const Lookup &l) {
        if (Ptr p = lookup(l))
            remove(p);
    }
};

}  

#endif
