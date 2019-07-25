









































#ifndef jshashtable_h_
#define jshashtable_h_

#include "jstl.h"

namespace js {


typedef uint32 HashNumber;

namespace detail {


template <class T, class HashPolicy, class AllocPolicy>
class HashTable : AllocPolicy
{
    typedef typename tl::StripConst<T>::result NonConstT;
    typedef typename HashPolicy::KeyType Key;
    typedef typename HashPolicy::Lookup Lookup;

    




    static void assignT(NonConstT &dst, const T &src) { dst = src; }

  public:
    class Entry {
        HashNumber keyHash;

      public:
        Entry() : keyHash(0), t() {}
        void operator=(const Entry &rhs) { keyHash = rhs.keyHash; assignT(t, rhs.t); }

        NonConstT t;

        bool isFree() const           { return keyHash == 0; }
        void setFree()                { keyHash = 0; assignT(t, T()); }
        bool isRemoved() const        { return keyHash == 1; }
        void setRemoved()             { keyHash = 1; assignT(t, T()); }
        bool isLive() const           { return keyHash > 1; }
        void setLive(HashNumber hn)   { JS_ASSERT(hn > 1); keyHash = hn; }

        void setCollision()           { JS_ASSERT(keyHash > 1); keyHash |= sCollisionBit; }
        void unsetCollision()         { JS_ASSERT(keyHash > 1); keyHash &= ~sCollisionBit; }
        bool hasCollision() const     { JS_ASSERT(keyHash > 1); return keyHash & sCollisionBit; }
        bool matchHash(HashNumber hn) { return (keyHash & ~sCollisionBit) == hn; }
        HashNumber getKeyHash() const { JS_ASSERT(!hasCollision()); return keyHash; }
    };

    





    class Ptr
    {
        friend class HashTable;
        typedef void (Ptr::* ConvertibleToBool)();
        void nonNull() {}

        Entry *entry;

      protected:
        Ptr(Entry &entry) : entry(&entry) {}

      public:
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
        AddPtr(Entry &entry, HashNumber hn) : Ptr(entry), keyHash(hn) {}
        HashNumber keyHash;
    };

    





    class Range
    {
      protected:
        friend class HashTable;

        Range(Entry *c, Entry *e) : cur(c), end(e) {
            while (cur != end && !cur->isLive())
                ++cur;
        }

        Entry *cur, *end;

      public:
        bool empty() const {
            return cur == end;
        }

        T &front() const {
            JS_ASSERT(!empty());
            return cur->t;
        }

        void popFront() {
            JS_ASSERT(!empty());
            while (++cur != end && !cur->isLive());
        }
    };

    








    class Enum : public Range
    {
        friend class HashTable;

        HashTable &table;
        bool removed;

        
        Enum(const Enum &);
        void operator=(const Enum &);

      public:
        template<class Map>
        Enum(Map &map) : Range(map.all()), table(map.impl), removed(false) {}

        








        void removeFront() {
            table.remove(*this->cur);
            removed = true;
        }

        
        ~Enum() {
            if (removed)
                table.checkUnderloaded();
        }

        
        void endEnumeration() {
            if (removed) {
                table.checkUnderloaded();
                removed = false;
            }
        }
    };

  private:
    uint32      hashShift;      
    uint32      tableCapacity;  
    uint32      entryCount;     
    uint32      gen;            
    uint32      removedCount;   
    Entry       *table;         

    void setTableSizeLog2(unsigned sizeLog2) {
        hashShift = sHashBits - sizeLog2;
        tableCapacity = JS_BIT(sizeLog2);
    }

#ifdef DEBUG
    mutable struct Stats {
        uint32          searches;       
        uint32          steps;          
        uint32          hits;           
        uint32          misses;         
        uint32          addOverRemoved; 
        uint32          removes;        
        uint32          removeFrees;    
        uint32          grows;          
        uint32          shrinks;        
        uint32          compresses;     
    } stats;
#   define METER(x) x
#else
#   define METER(x)
#endif

#ifdef DEBUG
    friend class js::ReentrancyGuard;
    mutable bool entered;
#endif

    static const unsigned sMinSizeLog2  = 4;
    static const unsigned sMinSize      = 1 << sMinSizeLog2;
    static const unsigned sSizeLimit    = JS_BIT(24);
    static const unsigned sHashBits     = tl::BitSize<HashNumber>::result;
    static const unsigned sGoldenRatio  = 0x9E3779B9U;       
    static const uint8    sMinAlphaFrac = 64;  
    static const uint8    sMaxAlphaFrac = 192; 
    static const uint8    sInvMaxAlpha  = 171; 
    static const unsigned sCollisionBit = 1;

    static Entry *createTable(AllocPolicy &alloc, uint32 capacity)
    {
        Entry *newTable = (Entry *)alloc.malloc(capacity * sizeof(Entry));
        if (!newTable)
            return NULL;
        for (Entry *e = newTable, *end = e + capacity; e != end; ++e)
            new(e) Entry();
        return newTable;
    }

    static void destroyTable(AllocPolicy &alloc, Entry *oldTable, uint32 capacity)
    {
        for (Entry *e = oldTable, *end = e + capacity; e != end; ++e)
            e->~Entry();
        alloc.free(oldTable);
    }

  public:
    HashTable(AllocPolicy ap)
      : AllocPolicy(ap),
        entryCount(0),
        gen(0),
        removedCount(0),
        table(NULL)
#ifdef DEBUG
        , entered(false)
#endif
    {}

    bool init(uint32 length)
    {
        



        JS_ASSERT(length < (uint32(1) << 23));
        uint32 capacity = (length * sInvMaxAlpha) >> 7;

        if (capacity < sMinSize)
            capacity = sMinSize;

        
        uint32 roundUp = sMinSize, roundUpLog2 = sMinSizeLog2;
        while (roundUp < capacity) {
            roundUp <<= 1;
            ++roundUpLog2;
        }

        capacity = roundUp;
        if (capacity >= sSizeLimit)
            return false;

        table = createTable(*this, capacity);
        if (!table)
            return false;

        setTableSizeLog2(roundUpLog2);
        METER(memset(&stats, 0, sizeof(stats)));
        return true;
    }

    bool initialized() const { return !!table; }

    ~HashTable()
    {
        if (table)
            destroyTable(*this, table, tableCapacity);
    }

  private:
    static uint32 hash1(uint32 hash0, uint32 shift) {
        return hash0 >> shift;
    }

    static uint32 hash2(uint32 hash0, uint32 log2, uint32 shift) {
        return ((hash0 << log2) >> shift) | 1;
    }

    bool overloaded() {
        return entryCount + removedCount >= ((sMaxAlphaFrac * tableCapacity) >> 8);
    }

    bool underloaded() {
        return tableCapacity > sMinSize &&
               entryCount <= ((sMinAlphaFrac * tableCapacity) >> 8);
    }

    static bool match(Entry &e, const Lookup &l) {
        return HashPolicy::match(HashPolicy::getKey(e.t), l);
    }

    struct SetCollisions {
        static void collide(Entry &e) { e.setCollision(); }
    };

    struct IgnoreCollisions {
        static void collide(Entry &) {}
    };

    template <class Op>
    AddPtr lookup(const Lookup &l, HashNumber keyHash) const
    {
        JS_ASSERT(table);
        METER(stats.searches++);

        
        keyHash *= sGoldenRatio;

        
        if (keyHash < 2)
            keyHash -= 2;
        keyHash &= ~sCollisionBit;

        
        uint32 h1 = hash1(keyHash, hashShift);
        Entry *entry = &table[h1];

        
        if (entry->isFree()) {
            METER(stats.misses++);
            return AddPtr(*entry, keyHash);
        }

        
        if (entry->matchHash(keyHash) && match(*entry, l)) {
            METER(stats.hits++);
            return AddPtr(*entry, keyHash);
        }

        
        unsigned sizeLog2 = sHashBits - hashShift;
        uint32 h2 = hash2(keyHash, sizeLog2, hashShift);
        uint32 sizeMask = JS_BITMASK(sizeLog2);

        
        Entry *firstRemoved = NULL;

        while(true) {
            if (JS_UNLIKELY(entry->isRemoved())) {
                if (!firstRemoved)
                    firstRemoved = entry;
            } else {
                Op::collide(*entry);
            }

            METER(stats.steps++);
            h1 -= h2;
            h1 &= sizeMask;

            entry = &table[h1];
            if (entry->isFree()) {
                METER(stats.misses++);
                return AddPtr(*(firstRemoved ? firstRemoved : entry), keyHash);
            }

            if (entry->matchHash(keyHash) && match(*entry, l)) {
                METER(stats.hits++);
                return AddPtr(*entry, keyHash);
            }
        }
    }

    







    Entry &findFreeEntry(HashNumber keyHash)
    {
        METER(stats.searches++);
        JS_ASSERT(!(keyHash & sCollisionBit));

        

        
        uint32 h1 = hash1(keyHash, hashShift);
        Entry *entry = &table[h1];

        
        if (entry->isFree()) {
            METER(stats.misses++);
            return *entry;
        }

        
        unsigned sizeLog2 = sHashBits - hashShift;
        uint32 h2 = hash2(keyHash, sizeLog2, hashShift);
        uint32 sizeMask = JS_BITMASK(sizeLog2);

        while(true) {
            JS_ASSERT(!entry->isRemoved());
            entry->setCollision();

            METER(stats.steps++);
            h1 -= h2;
            h1 &= sizeMask;

            entry = &table[h1];
            if (entry->isFree()) {
                METER(stats.misses++);
                return *entry;
            }
        }
    }

    bool changeTableSize(int deltaLog2)
    {
        
        Entry *oldTable = table;
        uint32 oldCap = tableCapacity;
        uint32 newLog2 = sHashBits - hashShift + deltaLog2;
        uint32 newCapacity = JS_BIT(newLog2);
        if (newCapacity >= sSizeLimit)
            return false;

        Entry *newTable = createTable(*this, newCapacity);
        if (!newTable)
            return false;

        
        setTableSizeLog2(newLog2);
        removedCount = 0;
        gen++;
        table = newTable;

        
        for (Entry *src = oldTable, *end = src + oldCap; src != end; ++src) {
            if (src->isLive()) {
                src->unsetCollision();
                findFreeEntry(src->getKeyHash()) = *src;
            }
        }

        destroyTable(*this, oldTable, oldCap);
        return true;
    }

    void remove(Entry &e)
    {
        METER(stats.removes++);
        if (e.hasCollision()) {
            e.setRemoved();
            removedCount++;
        } else {
            METER(stats.removeFrees++);
            e.setFree();
        }
        entryCount--;
    }

    void checkUnderloaded()
    {
        if (underloaded()) {
            METER(stats.shrinks++);
            (void) changeTableSize(-1);
        }
    }

  public:
    void clear()
    {
        for (Entry *e = table, *end = table + tableCapacity; e != end; ++e)
            *e = Entry();
        removedCount = 0;
        entryCount = 0;
    }

    Range all() const {
        return Range(table, table + tableCapacity);
    }

    bool empty() const {
        return !entryCount;
    }

    uint32 count() const{
        return entryCount;
    }

    uint32 generation() const {
        return gen;
    }

    Ptr lookup(const Lookup &l) const {
        ReentrancyGuard g(*this);
        return lookup<IgnoreCollisions>(l, HashPolicy::hash(l));
    }

    AddPtr lookupForAdd(const Lookup &l) const {
        ReentrancyGuard g(*this);
        return lookup<SetCollisions>(l, HashPolicy::hash(l));
    }

    bool add(AddPtr &p, const T &t)
    {
        ReentrancyGuard g(*this);
        JS_ASSERT(table);
        JS_ASSERT(!p.found());
        JS_ASSERT(!(p.keyHash & sCollisionBit));

        



        if (p.entry->isRemoved()) {
            METER(stats.addOverRemoved++);
            removedCount--;
            p.keyHash |= sCollisionBit;
        } else {
            
            if (overloaded()) {
                
                int deltaLog2;
                if (removedCount >= (tableCapacity >> 2)) {
                    METER(stats.compresses++);
                    deltaLog2 = 0;
                } else {
                    METER(stats.grows++);
                    deltaLog2 = 1;
                }

                if (!changeTableSize(deltaLog2))
                    return false;

                
                p.entry = &findFreeEntry(p.keyHash);
            }
        }

        p.entry->t = t;
        p.entry->setLive(p.keyHash);
        entryCount++;
        return true;
    }

    void remove(Ptr p)
    {
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
    static uint32 hash(const Lookup &l) {
        
        return l;
    }
    static bool match(const Key &k, const Lookup &l) {
        
        return k == l;
    }
};


template <class T>
struct DefaultHasher<T *>
{
    typedef T *Lookup;
    static uint32 hash(T *l) {
        



        return (uint32)(unsigned long)l >> 2;
    }
    static bool match(T *k, T *l) {
        return k == l;
    }
};

















template <class Key, class Value, class HashPolicy, class AllocPolicy>
class HashMap
{
  public:
    typedef typename HashPolicy::Lookup Lookup;

    class Entry
    {
        template <class, class, class> friend class detail::HashTable;
        void operator=(const Entry &rhs) {
            const_cast<Key &>(key) = rhs.key;
            value = rhs.value;
        }

      public:
        Entry() : key(), value() {}
        Entry(const Key &k, const Value &v) : key(k), value(v) {}

        const Key key;
        Value value;
    };

  private:
    
    struct MapHashPolicy : HashPolicy
    {
        typedef Key KeyType;
        static const Key &getKey(Entry &e) { return e.key; }
    };
    typedef detail::HashTable<Entry, MapHashPolicy, AllocPolicy> Impl;

    friend class Impl::Enum;

    
    HashMap(const HashMap &);
    HashMap &operator=(const HashMap &);

    Impl impl;

  public:
    



    HashMap(AllocPolicy a = AllocPolicy()) : impl(a) {}
    bool init(uint32 len = 0)                         { return impl.init(len); }
    bool initialized() const                          { return impl.initialized(); }

    












    typedef typename Impl::Ptr Ptr;
    Ptr lookup(const Lookup &l) const                 { return impl.lookup(l); }

    
    void remove(Ptr p)                                { impl.remove(p); }

    

















    typedef typename Impl::AddPtr AddPtr;
    AddPtr lookupForAdd(const Lookup &l) const        { return impl.lookupForAdd(l); }
    bool add(AddPtr &p, const Key &k, const Value &v) { return impl.add(p,Entry(k,v)); }

    









    typedef typename Impl::Range Range;
    Range all() const                                 { return impl.all(); }
    size_t count() const                              { return impl.count(); }

    












    typedef typename Impl::Enum Enum;

    
    void clear()                                      { impl.clear(); }

    
    bool empty() const                                { return impl.empty(); }

    



    unsigned generation() const                       { return impl.generation(); }

    

    bool has(const Lookup &l) const {
        return impl.lookup(l) != NULL;
    }

    Entry *put(const Key &k, const Value &v) {
        AddPtr p = lookupForAdd(k);
        if (p) {
            p->value = v;
            return &*p;
        }
        return add(p, k, v) ? &*p : NULL;
    }

    void remove(const Lookup &l) {
        if (Ptr p = lookup(l))
            remove(p);
    }
};

















template <class T, class HashPolicy, class AllocPolicy>
class HashSet
{
    typedef typename HashPolicy::Lookup Lookup;

    
    struct SetOps : HashPolicy {
        typedef T KeyType;
        static const KeyType &getKey(const T &t) { return t; }
    };
    typedef detail::HashTable<const T, SetOps, AllocPolicy> Impl;

    friend class Impl::Enum;

    
    HashSet(const HashSet &);
    HashSet &operator=(const HashSet &);

    Impl impl;

  public:
    



    HashSet(AllocPolicy a = AllocPolicy()) : impl(a) {}
    bool init(uint32 len = 0)                         { return impl.init(len); }
    bool initialized() const                          { return impl.initialized(); }

    










    typedef typename Impl::Ptr Ptr;
    Ptr lookup(const Lookup &l) const                 { return impl.lookup(l); }

    
    void remove(Ptr p)                                { impl.remove(p); }

    















    typedef typename Impl::AddPtr AddPtr;
    AddPtr lookupForAdd(const Lookup &l) const        { return impl.lookupForAdd(l); }
    bool add(AddPtr &p, const T &t)                   { return impl.add(p,t); }

    









    typedef typename Impl::Range Range;
    Range all() const                                 { return impl.all(); }
    size_t count() const                              { return impl.count(); }

    












    typedef typename Impl::Enum Enum;

    
    void clear()                                      { impl.clear(); }

    
    bool empty() const                                { return impl.empty(); }

    



    unsigned generation() const                       { return impl.generation(); }

    

    bool has(const Lookup &l) const {
        return impl.lookup(l) != NULL;
    }

    const T *put(const T &t) {
        AddPtr p = lookupForAdd(t);
        return p ? &*p : (add(p, t) ? &*p : NULL);
    }

    void remove(const Lookup &l) {
        if (Ptr p = lookup(l))
            remove(p);
    }
};

}  

#endif
