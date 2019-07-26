






#ifndef js_HashTable_h__
#define js_HashTable_h__

#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/TypeTraits.h"
#include "mozilla/Util.h"

#include "js/TemplateLib.h"
#include "js/Utility.h"

namespace js {

class TempAllocPolicy;
template <class> struct DefaultHasher;
template <class, class> class HashMapEntry;
namespace detail {
    template <class T> class HashTableEntry;
    template <class T, class HashPolicy, class AllocPolicy> class HashTable;
}


















template <class Key,
          class Value,
          class HashPolicy = DefaultHasher<Key>,
          class AllocPolicy = TempAllocPolicy>
class HashMap
{
    typedef HashMapEntry<Key, Value> TableEntry;

    struct MapHashPolicy : HashPolicy
    {
        typedef Key KeyType;
        static const Key &getKey(TableEntry &e) { return e.key; }
        static void setKey(TableEntry &e, Key &k) { const_cast<Key &>(e.key) = k; }
    };

    typedef detail::HashTable<TableEntry, MapHashPolicy, AllocPolicy> Impl;
    Impl impl;

  public:
    typedef typename HashPolicy::Lookup Lookup;
    typedef TableEntry Entry;

    
    
    HashMap(AllocPolicy a = AllocPolicy()) : impl(a)  {}
    bool init(uint32_t len = 16)                      { return impl.init(len); }
    bool initialized() const                          { return impl.initialized(); }

    
    
    
    
    
    
    
    
    
    
    
    typedef typename Impl::Ptr Ptr;
    Ptr lookup(const Lookup &l) const                 { return impl.lookup(l); }

    
    
    Ptr readonlyThreadsafeLookup(const Lookup &l) const { return impl.readonlyThreadsafeLookup(l); }

    
    void remove(Ptr p)                                { impl.remove(p); }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    typedef typename Impl::AddPtr AddPtr;
    AddPtr lookupForAdd(const Lookup &l) const {
        return impl.lookupForAdd(l);
    }

    template<typename KeyInput, typename ValueInput>
    bool add(AddPtr &p, const KeyInput &k, const ValueInput &v) {
        Entry e(k, v);
        return impl.add(p, Move(e));
    }

    bool add(AddPtr &p, const Key &k) {
        Entry e(k, Value());
        return impl.add(p, Move(e));
    }

    template<typename KeyInput, typename ValueInput>
    bool relookupOrAdd(AddPtr &p, const KeyInput &k, const ValueInput &v) {
        Entry e(k, v);
        return impl.relookupOrAdd(p, k, Move(e));
    }

    
    
    
    
    
    
    
    
    typedef typename Impl::Range Range;
    Range all() const                                 { return impl.all(); }

    
    
    
    
    
    
    
    
    
    
    
    typedef typename Impl::Enum Enum;

    
    
    void clear()                                      { impl.clear(); }

    
    void clearWithoutCallingDestructors()             { impl.clearWithoutCallingDestructors(); }

    
    
    void finish()                                     { impl.finish(); }

    
    bool empty() const                                { return impl.empty(); }

    
    uint32_t count() const                            { return impl.count(); }

    
    
    size_t capacity() const                           { return impl.capacity(); }

    
    
    size_t sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf) const {
        return impl.sizeOfExcludingThis(mallocSizeOf);
    }
    size_t sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf) const {
        return mallocSizeOf(this) + impl.sizeOfExcludingThis(mallocSizeOf);
    }

    
    
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

    
    template<typename KeyInput, typename ValueInput>
    bool putNew(const KeyInput &k, const ValueInput &v) {
        Entry e(k, v);
        return impl.putNew(k, Move(e));
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

    
    HashMap(MoveRef<HashMap> rhs) : impl(Move(rhs->impl)) {}
    void operator=(MoveRef<HashMap> rhs) { impl = Move(rhs->impl); }

  private:
    
    HashMap(const HashMap &hm) MOZ_DELETE;
    HashMap &operator=(const HashMap &hm) MOZ_DELETE;

    friend class Impl::Enum;

    typedef typename tl::StaticAssert<tl::IsRelocatableHeapType<Key>::result>::result keyAssert;
    typedef typename tl::StaticAssert<tl::IsRelocatableHeapType<Value>::result>::result valAssert;
};


















template <class T,
          class HashPolicy = DefaultHasher<T>,
          class AllocPolicy = TempAllocPolicy>
class HashSet
{
    struct SetOps : HashPolicy
    {
        typedef T KeyType;
        static const KeyType &getKey(const T &t) { return t; }
        static void setKey(T &t, KeyType &k) { t = k; }
    };

    typedef detail::HashTable<const T, SetOps, AllocPolicy> Impl;
    Impl impl;

  public:
    typedef typename HashPolicy::Lookup Lookup;
    typedef T Entry;

    
    
    HashSet(AllocPolicy a = AllocPolicy()) : impl(a)  {}
    bool init(uint32_t len = 16)                      { return impl.init(len); }
    bool initialized() const                          { return impl.initialized(); }

    
    
    
    
    
    
    
    
    
    typedef typename Impl::Ptr Ptr;
    Ptr lookup(const Lookup &l) const                 { return impl.lookup(l); }

    
    void remove(Ptr p)                                { impl.remove(p); }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    typedef typename Impl::AddPtr AddPtr;
    AddPtr lookupForAdd(const Lookup &l) const        { return impl.lookupForAdd(l); }

    bool add(AddPtr &p, const T &t)                   { return impl.add(p, t); }

    bool relookupOrAdd(AddPtr &p, const Lookup &l, const T &t) {
        return impl.relookupOrAdd(p, l, t);
    }

    
    
    
    
    
    
    
    
    typedef typename Impl::Range Range;
    Range all() const                                 { return impl.all(); }

    
    
    
    
    
    
    
    
    
    
    
    typedef typename Impl::Enum Enum;

    
    
    void clear()                                      { impl.clear(); }

    
    
    void finish()                                     { impl.finish(); }

    
    bool empty() const                                { return impl.empty(); }

    
    uint32_t count() const                            { return impl.count(); }

    
    
    size_t capacity() const                           { return impl.capacity(); }

    
    
    size_t sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf) const {
        return impl.sizeOfExcludingThis(mallocSizeOf);
    }
    size_t sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf) const {
        return mallocSizeOf(this) + impl.sizeOfExcludingThis(mallocSizeOf);
    }

    
    
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

    
    HashSet(MoveRef<HashSet> rhs) : impl(Move(rhs->impl)) {}
    void operator=(MoveRef<HashSet> rhs) { impl = Move(rhs->impl); }

  private:
    
    HashSet(const HashSet &hs) MOZ_DELETE;
    HashSet &operator=(const HashSet &hs) MOZ_DELETE;

    friend class Impl::Enum;

    typedef typename tl::StaticAssert<tl::IsRelocatableHeapType<T>::result>::result _;
};






























template <typename Key, size_t zeroBits>
struct PointerHasher
{
    typedef Key Lookup;
    static HashNumber hash(const Lookup &l) {
        JS_ASSERT(!JS::IsPoisonedPtr(l));
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
        JS_ASSERT(!JS::IsPoisonedPtr(k));
        JS_ASSERT(!JS::IsPoisonedPtr(l));
        return k == l;
    }
};





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



template <class T>
struct DefaultHasher<T *> : PointerHasher<T *, tl::FloorLog2<sizeof(void *)>::result>
{};


template <>
struct DefaultHasher<double>
{
    typedef double Lookup;
    static HashNumber hash(double d) {
        JS_STATIC_ASSERT(sizeof(HashNumber) == 4);
        union {
            struct {
                uint32_t lo;
                uint32_t hi;
            } s;
            double d;
        } u;
        u.d = d;
        return u.s.lo ^ u.s.hi;
    }
    static bool match(double lhs, double rhs) {
        return lhs == rhs;
    }
};








template <class Key, class Value>
class HashMapEntry
{
    template <class, class, class> friend class detail::HashTable;
    template <class> friend class detail::HashTableEntry;

    HashMapEntry(const HashMapEntry &) MOZ_DELETE;
    void operator=(const HashMapEntry &) MOZ_DELETE;

  public:
    template<typename KeyInput, typename ValueInput>
    HashMapEntry(const KeyInput &k, const ValueInput &v) : key(k), value(v) {}

    HashMapEntry(MoveRef<HashMapEntry> rhs)
      : key(Move(rhs->key)), value(Move(rhs->value)) { }

    const Key key;
    Value value;
};

} 

namespace mozilla {

template <typename T>
struct IsPod<js::detail::HashTableEntry<T> > : IsPod<T> {};

template <typename K, typename V>
struct IsPod<js::HashMapEntry<K, V> >
  : IntegralConstant<bool, IsPod<K>::value && IsPod<V>::value>
{};

} 

namespace js {

namespace detail {

template <class T, class HashPolicy, class AllocPolicy>
class HashTable;

template <class T>
class HashTableEntry
{
    template <class, class, class> friend class HashTable;
    typedef typename tl::StripConst<T>::result NonConstT;

    HashNumber keyHash;
    mozilla::AlignedStorage2<NonConstT> mem;

    static const HashNumber sFreeKey = 0;
    static const HashNumber sRemovedKey = 1;
    static const HashNumber sCollisionBit = 1;

    
    JS_STATIC_ASSERT(sFreeKey == 0);

    static bool isLiveHash(HashNumber hash)
    {
        return hash > sRemovedKey;
    }

    HashTableEntry(const HashTableEntry &) MOZ_DELETE;
    void operator=(const HashTableEntry &) MOZ_DELETE;
    ~HashTableEntry() MOZ_DELETE;

  public:
    

    void destroyIfLive() {
        if (isLive())
            mem.addr()->~T();
    }

    void destroy() {
        JS_ASSERT(isLive());
        mem.addr()->~T();
    }

    void swap(HashTableEntry *other) {
        Swap(keyHash, other->keyHash);
        Swap(mem, other->mem);
    }

    T &get() { JS_ASSERT(isLive()); return *mem.addr(); }

    bool isFree() const    { return keyHash == sFreeKey; }
    void clearLive()       { JS_ASSERT(isLive()); keyHash = sFreeKey; mem.addr()->~T(); }
    void clear()           { if (isLive()) mem.addr()->~T(); keyHash = sFreeKey; }
    void clearNoDtor()     { keyHash = sFreeKey; }
    bool isRemoved() const { return keyHash == sRemovedKey; }
    void removeLive()      { JS_ASSERT(isLive()); keyHash = sRemovedKey; mem.addr()->~T(); }
    bool isLive() const    { return isLiveHash(keyHash); }
    void setCollision()               { JS_ASSERT(isLive()); keyHash |= sCollisionBit; }
    void setCollision(HashNumber bit) { JS_ASSERT(isLive()); keyHash |= bit; }
    void unsetCollision()             { keyHash &= ~sCollisionBit; }
    bool hasCollision() const         { return keyHash & sCollisionBit; }
    bool matchHash(HashNumber hn)     { return (keyHash & ~sCollisionBit) == hn; }
    HashNumber getKeyHash() const     { return keyHash & ~sCollisionBit; }

    template <class U>
    void setLive(HashNumber hn, const U &u)
    {
        JS_ASSERT(!isLive());
        keyHash = hn;
        new(mem.addr()) T(u);
        JS_ASSERT(isLive());
    }
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

        Entry *entry_;

      protected:
        Ptr(Entry &entry) : entry_(&entry) {}

      public:
        
        Ptr() {
#ifdef DEBUG
            entry_ = (Entry *)0xbad;
#endif
        }

        bool found() const                    { return entry_->isLive(); }
        operator ConvertibleToBool() const    { return found() ? &Ptr::nonNull : 0; }
        bool operator==(const Ptr &rhs) const { JS_ASSERT(found() && rhs.found()); return entry_ == rhs.entry_; }
        bool operator!=(const Ptr &rhs) const { return !(*this == rhs); }

        T &operator*() const                  { return entry_->get(); }
        T *operator->() const                 { return &entry_->get(); }
    };

    
    class AddPtr : public Ptr
    {
        friend class HashTable;
        HashNumber keyHash;
        mozilla::DebugOnly<uint64_t> mutationCount;

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
        mozilla::DebugOnly<bool> validEntry;

      public:
        Range() : cur(NULL), end(NULL), validEntry(false) {}

        bool empty() const {
            return cur == end;
        }

        T &front() const {
            JS_ASSERT(validEntry);
            JS_ASSERT(!empty());
            return cur->get();
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
            typename HashTableEntry<T>::NonConstT t(Move(this->cur->get()));
            HashPolicy::setKey(t, const_cast<Key &>(k));
            table.remove(*this->cur);
            table.putNewInfallible(l, Move(t));
            rekeyed = true;
            this->validEntry = false;
        }

        void rekeyFront(const Key &k) {
            rekeyFront(k, k);
        }

        
        ~Enum() {
            if (rekeyed) {
                table.gen++;
                table.checkOverRemoved();
            }

            if (removed)
                table.compactIfUnderloaded();
        }
    };

    
    HashTable(MoveRef<HashTable> rhs)
      : AllocPolicy(*rhs)
    {
        PodAssign(this, &*rhs);
        rhs->table = NULL;
    }
    void operator=(MoveRef<HashTable> rhs) {
        if (table)
            destroyTable(*this, table, capacity());
        PodAssign(this, &*rhs);
        rhs->table = NULL;
    }

  private:
    
    HashTable(const HashTable &) MOZ_DELETE;
    void operator=(const HashTable &) MOZ_DELETE;

  private:
    uint32_t    hashShift;      
    uint32_t    entryCount;     
    uint32_t    gen;            
    uint32_t    removedCount;   
    Entry       *table;         

    void setTableSizeLog2(unsigned sizeLog2)
    {
        hashShift = sHashBits - sizeLog2;
    }

#ifdef DEBUG
    mutable struct Stats
    {
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
    mutable mozilla::DebugOnly<bool> entered;
    mozilla::DebugOnly<uint64_t>     mutationCount;

    
    
    static const unsigned sMinCapacityLog2 = 2;
    static const unsigned sMinCapacity  = 1 << sMinCapacityLog2;
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
        
        return (Entry *)alloc.calloc_(capacity * sizeof(Entry));
    }

    static void destroyTable(AllocPolicy &alloc, Entry *oldTable, uint32_t capacity)
    {
        for (Entry *e = oldTable, *end = e + capacity; e < end; ++e)
            e->destroyIfLive();
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
        JS_ASSERT(!initialized());

        
        
        if (length > sMaxInit) {
            this->reportAllocOverflow();
            return false;
        }
        uint32_t newCapacity = (length * sInvMaxAlpha) >> 7;

        if (newCapacity < sMinCapacity)
            newCapacity = sMinCapacity;

        
        uint32_t roundUp = sMinCapacity, roundUpLog2 = sMinCapacityLog2;
        while (roundUp < newCapacity) {
            roundUp <<= 1;
            ++roundUpLog2;
        }

        newCapacity = roundUp;
        JS_ASSERT(newCapacity <= sMaxCapacity);

        table = createTable(*this, newCapacity);
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
    HashNumber hash1(HashNumber hash0) const
    {
        return hash0 >> hashShift;
    }

    struct DoubleHash
    {
        HashNumber h2;
        HashNumber sizeMask;
    };

    DoubleHash hash2(HashNumber curKeyHash) const
    {
        unsigned sizeLog2 = sHashBits - hashShift;
        DoubleHash dh = {
            ((curKeyHash << sizeLog2) >> hashShift) | 1,
            (HashNumber(1) << sizeLog2) - 1
        };
        return dh;
    }

    static HashNumber applyDoubleHash(HashNumber h1, const DoubleHash &dh)
    {
        return (h1 - dh.h2) & dh.sizeMask;
    }

    bool overloaded()
    {
        return entryCount + removedCount >= ((sMaxAlphaFrac * capacity()) >> 8);
    }

    
    static bool wouldBeUnderloaded(uint32_t capacity, uint32_t entryCount)
    {
        return capacity > sMinCapacity && entryCount <= ((sMinAlphaFrac * capacity) >> 8);
    }

    bool underloaded()
    {
        return wouldBeUnderloaded(capacity(), entryCount);
    }

    static bool match(Entry &e, const Lookup &l)
    {
        return HashPolicy::match(HashPolicy::getKey(e.get()), l);
    }

    Entry &lookup(const Lookup &l, HashNumber keyHash, unsigned collisionBit) const
    {
        JS_ASSERT(isLiveHash(keyHash));
        JS_ASSERT(!(keyHash & sCollisionBit));
        JS_ASSERT(collisionBit == 0 || collisionBit == sCollisionBit);
        JS_ASSERT(table);
        METER(stats.searches++);

        
        HashNumber h1 = hash1(keyHash);
        Entry *entry = &table[h1];

        
        if (entry->isFree()) {
            METER(stats.misses++);
            return *entry;
        }

        
        if (entry->matchHash(keyHash) && match(*entry, l)) {
            METER(stats.hits++);
            return *entry;
        }

        
        DoubleHash dh = hash2(keyHash);

        
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

        

        
        HashNumber h1 = hash1(keyHash);
        Entry *entry = &table[h1];

        
        if (!entry->isLive()) {
            METER(stats.misses++);
            return *entry;
        }

        
        DoubleHash dh = hash2(keyHash);

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
                HashNumber hn = src->getKeyHash();
                findFreeEntry(hn).setLive(hn, Move(src->get()));
                src->destroy();
            }
        }

        
        this->free_(oldTable);
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
            if (checkOverloaded() == RehashFailed)
                rehashTableInPlace();
        }
    }

    void remove(Entry &e)
    {
        JS_ASSERT(table);
        METER(stats.removes++);

        if (e.hasCollision()) {
            e.removeLive();
            removedCount++;
        } else {
            METER(stats.removeFrees++);
            e.clearLive();
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

    
    
    
    void compactIfUnderloaded()
    {
        int32_t resizeLog2 = 0;
        uint32_t newCapacity = capacity();
        while (wouldBeUnderloaded(newCapacity, entryCount)) {
            newCapacity = newCapacity >> 1;
            resizeLog2--;
        }

        if (resizeLog2 != 0) {
            changeTableSize(resizeLog2);
        }
    }

    
    
    
    
    
    void rehashTableInPlace()
    {
        METER(stats.rehashes++);
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
            HashNumber h1 = hash1(keyHash);
            DoubleHash dh = hash2(keyHash);
            Entry *tgt = &table[h1];
            while (true) {
                if (!tgt->hasCollision()) {
                    src->swap(tgt);
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
        if (mozilla::IsPod<Entry>::value) {
            memset(table, 0, sizeof(*table) * capacity());
        } else {
            uint32_t tableCapacity = capacity();
            for (Entry *e = table, *end = table + tableCapacity; e < end; ++e)
                e->clear();
        }
        removedCount = 0;
        entryCount = 0;
        mutationCount++;
    }

    void clearWithoutCallingDestructors()
    {
        if (mozilla::IsPod<Entry>::value) {
            memset(table, 0, sizeof(*table) * capacity());
        } else {
            uint32_t tableCapacity = capacity();
            for (Entry *e = table, *end = table + tableCapacity; e < end; ++e)
                e->clearNoDtor();
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

    Range all() const
    {
        JS_ASSERT(table);
        return Range(table, table + capacity());
    }

    bool empty() const
    {
        JS_ASSERT(table);
        return !entryCount;
    }

    uint32_t count() const
    {
        JS_ASSERT(table);
        return entryCount;
    }

    uint32_t capacity() const
    {
        JS_ASSERT(table);
        return JS_BIT(sHashBits - hashShift);
    }

    uint32_t generation() const
    {
        JS_ASSERT(table);
        return gen;
    }

    size_t sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf) const
    {
        return mallocSizeOf(table);
    }

    size_t sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf) const
    {
        return mallocSizeOf(this) + sizeOfExcludingThis(mallocSizeOf);
    }

    Ptr lookup(const Lookup &l) const
    {
        ReentrancyGuard g(*this);
        HashNumber keyHash = prepareHash(l);
        return Ptr(lookup(l, keyHash, 0));
    }

    Ptr readonlyThreadsafeLookup(const Lookup &l) const
    {
        HashNumber keyHash = prepareHash(l);
        return Ptr(lookup(l, keyHash, 0));
    }

    AddPtr lookupForAdd(const Lookup &l) const
    {
        ReentrancyGuard g(*this);
        HashNumber keyHash = prepareHash(l);
        Entry &entry = lookup(l, keyHash, sCollisionBit);
        AddPtr p(entry, keyHash);
        p.mutationCount = mutationCount;
        return p;
    }

    template <class U>
    bool add(AddPtr &p, const U &rhs)
    {
        ReentrancyGuard g(*this);
        JS_ASSERT(mutationCount == p.mutationCount);
        JS_ASSERT(table);
        JS_ASSERT(!p.found());
        JS_ASSERT(!(p.keyHash & sCollisionBit));

        
        
        if (p.entry_->isRemoved()) {
            METER(stats.addOverRemoved++);
            removedCount--;
            p.keyHash |= sCollisionBit;
        } else {
            
            RebuildStatus status = checkOverloaded();
            if (status == RehashFailed)
                return false;
            if (status == Rehashed)
                p.entry_ = &findFreeEntry(p.keyHash);
        }

        p.entry_->setLive(p.keyHash, rhs);
        entryCount++;
        mutationCount++;
        return true;
    }

    template <class U>
    void putNewInfallible(const Lookup &l, const U &u)
    {
        JS_ASSERT(table);

        HashNumber keyHash = prepareHash(l);
        Entry *entry = &findFreeEntry(keyHash);

        if (entry->isRemoved()) {
            METER(stats.addOverRemoved++);
            removedCount--;
            keyHash |= sCollisionBit;
        }

        entry->setLive(keyHash, u);
        entryCount++;
        mutationCount++;
    }

    template <class U>
    bool putNew(const Lookup &l, const U &u)
    {
        if (checkOverloaded() == RehashFailed)
            return false;

        putNewInfallible(l, u);
        return true;
    }

    template <class U>
    bool relookupOrAdd(AddPtr& p, const Lookup &l, const U &u)
    {
        p.mutationCount = mutationCount;
        {
            ReentrancyGuard g(*this);
            p.entry_ = &lookup(l, p.keyHash, sCollisionBit);
        }
        return p.found() || add(p, u);
    }

    void remove(Ptr p)
    {
        JS_ASSERT(table);
        ReentrancyGuard g(*this);
        JS_ASSERT(p.found());
        remove(*p.entry_);
        checkUnderloaded();
    }

#undef METER
};

}  
}  

#endif  

