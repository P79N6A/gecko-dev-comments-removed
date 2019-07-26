





#include "builtin/MapObject.h"

#include "mozilla/Move.h"

#include "jscntxt.h"
#include "jsiter.h"
#include "jsobj.h"

#include "gc/Marking.h"
#include "js/Utility.h"
#include "vm/GlobalObject.h"
#include "vm/Interpreter.h"

#include "jsobjinlines.h"

using namespace js;

using mozilla::DoubleIsInt32;
using mozilla::IsNaN;
using mozilla::OldMove;
using mozilla::MoveRef;
using mozilla::ArrayLength;
using JS::DoubleNaNValue;
































namespace js {

namespace detail {






template <class T, class Ops, class AllocPolicy>
class OrderedHashTable
{
  public:
    typedef typename Ops::KeyType Key;
    typedef typename Ops::Lookup Lookup;

    struct Data
    {
        T element;
        Data *chain;

        Data(const T &e, Data *c) : element(e), chain(c) {}
        Data(MoveRef<T> e, Data *c) : element(e), chain(c) {}
    };

    class Range;
    friend class Range;

  private:
    Data **hashTable;           
    Data *data;                 
                                
    uint32_t dataLength;        
    uint32_t dataCapacity;      
    uint32_t liveCount;         
    uint32_t hashShift;         
    Range *ranges;              
    AllocPolicy alloc;

  public:
    OrderedHashTable(AllocPolicy &ap)
        : hashTable(nullptr), data(nullptr), dataLength(0), ranges(nullptr), alloc(ap) {}

    bool init() {
        MOZ_ASSERT(!hashTable, "init must be called at most once");

        uint32_t buckets = initialBuckets();
        Data **tableAlloc = static_cast<Data **>(alloc.malloc_(buckets * sizeof(Data *)));
        if (!tableAlloc)
            return false;
        for (uint32_t i = 0; i < buckets; i++)
            tableAlloc[i] = nullptr;

        uint32_t capacity = uint32_t(buckets * fillFactor());
        Data *dataAlloc = static_cast<Data *>(alloc.malloc_(capacity * sizeof(Data)));
        if (!dataAlloc) {
            alloc.free_(tableAlloc);
            return false;
        }

        
        
        hashTable = tableAlloc;
        data = dataAlloc;
        dataLength = 0;
        dataCapacity = capacity;
        liveCount = 0;
        hashShift = HashNumberSizeBits - initialBucketsLog2();
        MOZ_ASSERT(hashBuckets() == buckets);
        return true;
    }

    ~OrderedHashTable() {
        for (Range *r = ranges, *next; r; r = next) {
            next = r->next;
            r->onTableDestroyed();
        }
        alloc.free_(hashTable);
        freeData(data, dataLength);
    }

    
    uint32_t count() const { return liveCount; }

    
    bool has(const Lookup &l) const {
        return lookup(l) != nullptr;
    }

    
    T *get(const Lookup &l) {
        Data *e = lookup(l, prepareHash(l));
        return e ? &e->element : nullptr;
    }

    
    const T *get(const Lookup &l) const {
        return const_cast<OrderedHashTable *>(this)->get(l);
    }

    







    bool put(const T &element) {
        HashNumber h = prepareHash(Ops::getKey(element));
        if (Data *e = lookup(Ops::getKey(element), h)) {
            e->element = element;
            return true;
        }

        if (dataLength == dataCapacity) {
            
            
            uint32_t newHashShift = liveCount >= dataCapacity * 0.75 ? hashShift - 1 : hashShift;
            if (!rehash(newHashShift))
                return false;
        }

        h >>= hashShift;
        liveCount++;
        Data *e = &data[dataLength++];
        new (e) Data(element, hashTable[h]);
        hashTable[h] = e;
        return true;
    }

    








    bool remove(const Lookup &l, bool *foundp) {
        
        
        

        
        Data *e = lookup(l, prepareHash(l));
        if (e == nullptr) {
            *foundp = false;
            return true;
        }

        *foundp = true;
        liveCount--;
        Ops::makeEmpty(&e->element);

        
        uint32_t pos = e - data;
        for (Range *r = ranges; r; r = r->next)
            r->onRemove(pos);

        
        if (hashBuckets() > initialBuckets() && liveCount < dataLength * minDataFill()) {
            if (!rehash(hashShift + 1))
                return false;
        }
        return true;
    }

    









    bool clear() {
        if (dataLength != 0) {
            Data **oldHashTable = hashTable;
            Data *oldData = data;
            uint32_t oldDataLength = dataLength;

            hashTable = nullptr;
            if (!init()) {
                
                hashTable = oldHashTable;
                return false;
            }

            alloc.free_(oldHashTable);
            freeData(oldData, oldDataLength);
            for (Range *r = ranges; r; r = r->next)
                r->onClear();
        }

        MOZ_ASSERT(hashTable);
        MOZ_ASSERT(data);
        MOZ_ASSERT(dataLength == 0);
        MOZ_ASSERT(liveCount == 0);
        return true;
    }

    
































    class Range
    {
        friend class OrderedHashTable;

        OrderedHashTable &ht;

        
        uint32_t i;

        



        uint32_t count;

        









        Range **prevp;
        Range *next;

        



        Range(OrderedHashTable &ht) : ht(ht), i(0), count(0), prevp(&ht.ranges), next(ht.ranges) {
            *prevp = this;
            if (next)
                next->prevp = &next;
            seek();
        }

      public:
        Range(const Range &other)
            : ht(other.ht), i(other.i), count(other.count), prevp(&ht.ranges), next(ht.ranges)
        {
            *prevp = this;
            if (next)
                next->prevp = &next;
        }

        ~Range() {
            *prevp = next;
            if (next)
                next->prevp = prevp;
        }

      private:
        
        Range &operator=(const Range &other) MOZ_DELETE;

        void seek() {
            while (i < ht.dataLength && Ops::isEmpty(Ops::getKey(ht.data[i].element)))
                i++;
        }

        



        void onRemove(uint32_t j) {
            MOZ_ASSERT(valid());
            if (j < i)
                count--;
            if (j == i)
                seek();
        }

        





        void onCompact() {
            MOZ_ASSERT(valid());
            i = count;
        }

        
        void onClear() {
            MOZ_ASSERT(valid());
            i = count = 0;
        }

        bool valid() const {
            return next != this;
        }

        void onTableDestroyed() {
            MOZ_ASSERT(valid());
            prevp = &next;
            next = this;
        }

      public:
        bool empty() const {
            MOZ_ASSERT(valid());
            return i >= ht.dataLength;
        }

        







        T &front() {
            MOZ_ASSERT(valid());
            MOZ_ASSERT(!empty());
            return ht.data[i].element;
        }

        








        void popFront() {
            MOZ_ASSERT(valid());
            MOZ_ASSERT(!empty());
            MOZ_ASSERT(!Ops::isEmpty(Ops::getKey(ht.data[i].element)));
            count++;
            i++;
            seek();
        }

        






        void rekeyFront(const Key &k) {
            MOZ_ASSERT(valid());
            Data &entry = ht.data[i];
            HashNumber oldHash = prepareHash(Ops::getKey(entry.element)) >> ht.hashShift;
            HashNumber newHash = prepareHash(k) >> ht.hashShift;
            Ops::setKey(entry.element, k);
            if (newHash != oldHash) {
                
                
                
                
                
                Data **ep = &ht.hashTable[oldHash];
                while (*ep != &entry)
                    ep = &(*ep)->chain;
                *ep = entry.chain;

                
                
                
                
                
                
                ep = &ht.hashTable[newHash];
                while (*ep && *ep > &entry)
                    ep = &(*ep)->chain;
                entry.chain = *ep;
                *ep = &entry;
            }
        }
    };

    Range all() { return Range(*this); }

    






    void rekeyOneEntry(const Key &current, const Key &newKey, const T &element) {
        if (current == newKey)
            return;

        Data *entry = lookup(current, prepareHash(current));
        if (!entry)
            return;

        HashNumber oldHash = prepareHash(current) >> hashShift;
        HashNumber newHash = prepareHash(newKey) >> hashShift;

        entry->element = element;

        
        
        
        
        
        Data **ep = &hashTable[oldHash];
        while (*ep != entry)
            ep = &(*ep)->chain;
        *ep = entry->chain;

        
        
        
        
        
        
        ep = &hashTable[newHash];
        while (*ep && *ep > entry)
            ep = &(*ep)->chain;
        entry->chain = *ep;
        *ep = entry;
    }

  private:
    
    static uint32_t initialBucketsLog2() { return 1; }
    static uint32_t initialBuckets() { return 1 << initialBucketsLog2(); }

    









    static double fillFactor() { return 8.0 / 3.0; }

    



    static double minDataFill() { return 0.25; }

    static HashNumber prepareHash(const Lookup &l) {
        return ScrambleHashCode(Ops::hash(l));
    }

    
    uint32_t hashBuckets() const {
        return 1 << (HashNumberSizeBits - hashShift);
    }

    static void destroyData(Data *data, uint32_t length) {
        for (Data *p = data + length; p != data; )
            (--p)->~Data();
    }

    void freeData(Data *data, uint32_t length) {
        destroyData(data, length);
        alloc.free_(data);
    }

    Data *lookup(const Lookup &l, HashNumber h) {
        for (Data *e = hashTable[h >> hashShift]; e; e = e->chain) {
            if (Ops::match(Ops::getKey(e->element), l))
                return e;
        }
        return nullptr;
    }

    const Data *lookup(const Lookup &l) const {
        return const_cast<OrderedHashTable *>(this)->lookup(l, prepareHash(l));
    }

    
    void compacted() {
        
        
        for (Range *r = ranges; r; r = r->next)
            r->onCompact();
    }

    
    void rehashInPlace() {
        for (uint32_t i = 0, N = hashBuckets(); i < N; i++)
            hashTable[i] = nullptr;
        Data *wp = data, *end = data + dataLength;
        for (Data *rp = data; rp != end; rp++) {
            if (!Ops::isEmpty(Ops::getKey(rp->element))) {
                HashNumber h = prepareHash(Ops::getKey(rp->element)) >> hashShift;
                if (rp != wp)
                    wp->element = OldMove(rp->element);
                wp->chain = hashTable[h];
                hashTable[h] = wp;
                wp++;
            }
        }
        MOZ_ASSERT(wp == data + liveCount);

        while (wp != end)
            (--end)->~Data();
        dataLength = liveCount;
        compacted();
    }

    






    bool rehash(uint32_t newHashShift) {
        
        
        if (newHashShift == hashShift) {
            rehashInPlace();
            return true;
        }

        size_t newHashBuckets = 1 << (HashNumberSizeBits - newHashShift);
        Data **newHashTable = static_cast<Data **>(alloc.malloc_(newHashBuckets * sizeof(Data *)));
        if (!newHashTable)
            return false;
        for (uint32_t i = 0; i < newHashBuckets; i++)
            newHashTable[i] = nullptr;

        uint32_t newCapacity = uint32_t(newHashBuckets * fillFactor());
        Data *newData = static_cast<Data *>(alloc.malloc_(newCapacity * sizeof(Data)));
        if (!newData) {
            alloc.free_(newHashTable);
            return false;
        }

        Data *wp = newData;
        for (Data *p = data, *end = data + dataLength; p != end; p++) {
            if (!Ops::isEmpty(Ops::getKey(p->element))) {
                HashNumber h = prepareHash(Ops::getKey(p->element)) >> newHashShift;
                new (wp) Data(OldMove(p->element), newHashTable[h]);
                newHashTable[h] = wp;
                wp++;
            }
        }
        MOZ_ASSERT(wp == newData + liveCount);

        alloc.free_(hashTable);
        freeData(data, dataLength);

        hashTable = newHashTable;
        data = newData;
        dataLength = liveCount;
        dataCapacity = newCapacity;
        hashShift = newHashShift;
        MOZ_ASSERT(hashBuckets() == newHashBuckets);

        compacted();
        return true;
    }

    
    OrderedHashTable &operator=(const OrderedHashTable &) MOZ_DELETE;
    OrderedHashTable(const OrderedHashTable &) MOZ_DELETE;
};

}  

template <class Key, class Value, class OrderedHashPolicy, class AllocPolicy>
class OrderedHashMap
{
  public:
    class Entry
    {
        template <class, class, class> friend class detail::OrderedHashTable;
        void operator=(const Entry &rhs) {
            const_cast<Key &>(key) = rhs.key;
            value = rhs.value;
        }

        void operator=(MoveRef<Entry> rhs) {
            const_cast<Key &>(key) = OldMove(rhs->key);
            value = OldMove(rhs->value);
        }

      public:
        Entry() : key(), value() {}
        Entry(const Key &k, const Value &v) : key(k), value(v) {}
        Entry(MoveRef<Entry> rhs) : key(OldMove(rhs->key)), value(OldMove(rhs->value)) {}

        const Key key;
        Value value;
    };

  private:
    struct MapOps : OrderedHashPolicy
    {
        typedef Key KeyType;
        static void makeEmpty(Entry *e) {
            OrderedHashPolicy::makeEmpty(const_cast<Key *>(&e->key));

            
            
            e->value = Value();
        }
        static const Key &getKey(const Entry &e) { return e.key; }
        static void setKey(Entry &e, const Key &k) { const_cast<Key &>(e.key) = k; }
    };

    typedef detail::OrderedHashTable<Entry, MapOps, AllocPolicy> Impl;
    Impl impl;

  public:
    typedef typename Impl::Range Range;

    OrderedHashMap(AllocPolicy ap = AllocPolicy()) : impl(ap) {}
    bool init()                                     { return impl.init(); }
    uint32_t count() const                          { return impl.count(); }
    bool has(const Key &key) const                  { return impl.has(key); }
    Range all()                                     { return impl.all(); }
    const Entry *get(const Key &key) const          { return impl.get(key); }
    Entry *get(const Key &key)                      { return impl.get(key); }
    bool put(const Key &key, const Value &value)    { return impl.put(Entry(key, value)); }
    bool remove(const Key &key, bool *foundp)       { return impl.remove(key, foundp); }
    bool clear()                                    { return impl.clear(); }

    void rekeyOneEntry(const Key &current, const Key &newKey) {
        const Entry *e = get(current);
        if (!e)
            return;
        return impl.rekeyOneEntry(current, newKey, Entry(newKey, e->value));
    }
};

template <class T, class OrderedHashPolicy, class AllocPolicy>
class OrderedHashSet
{
  private:
    struct SetOps : OrderedHashPolicy
    {
        typedef const T KeyType;
        static const T &getKey(const T &v) { return v; }
        static void setKey(const T &e, const T &v) { const_cast<T &>(e) = v; }
    };

    typedef detail::OrderedHashTable<T, SetOps, AllocPolicy> Impl;
    Impl impl;

  public:
    typedef typename Impl::Range Range;

    OrderedHashSet(AllocPolicy ap = AllocPolicy()) : impl(ap) {}
    bool init()                                     { return impl.init(); }
    uint32_t count() const                          { return impl.count(); }
    bool has(const T &value) const                  { return impl.has(value); }
    Range all()                                     { return impl.all(); }
    bool put(const T &value)                        { return impl.put(value); }
    bool remove(const T &value, bool *foundp)       { return impl.remove(value, foundp); }
    bool clear()                                    { return impl.clear(); }

    void rekeyOneEntry(const T &current, const T &newKey) {
        return impl.rekeyOneEntry(current, newKey, newKey);
    }
};

}  




bool
HashableValue::setValue(JSContext *cx, HandleValue v)
{
    if (v.isString()) {
        
        JSString *str = AtomizeString(cx, v.toString(), DoNotInternAtom);
        if (!str)
            return false;
        value = StringValue(str);
    } else if (v.isDouble()) {
        double d = v.toDouble();
        int32_t i;
        if (DoubleIsInt32(d, &i)) {
            
            value = Int32Value(i);
        } else if (IsNaN(d)) {
            
            value = DoubleNaNValue();
        } else {
            value = v;
        }
    } else {
        value = v;
    }

    JS_ASSERT(value.isUndefined() || value.isNull() || value.isBoolean() ||
              value.isNumber() || value.isString() || value.isObject());
    return true;
}

HashNumber
HashableValue::hash() const
{
    
    
    
    return value.asRawBits();
}

bool
HashableValue::operator==(const HashableValue &other) const
{
    
    bool b = (value.asRawBits() == other.value.asRawBits());

#ifdef DEBUG
    bool same;
    JS_ASSERT(SameValue(nullptr, value, other.value, &same));
    JS_ASSERT(same == b);
#endif
    return b;
}

HashableValue
HashableValue::mark(JSTracer *trc) const
{
    HashableValue hv(*this);
    JS_SET_TRACING_LOCATION(trc, (void *)this);
    gc::MarkValue(trc, &hv.value, "key");
    return hv;
}




namespace {

class MapIteratorObject : public JSObject
{
  public:
    static const Class class_;

    enum { TargetSlot, KindSlot, RangeSlot, SlotCount };
    static const JSFunctionSpec methods[];
    static MapIteratorObject *create(JSContext *cx, HandleObject mapobj, ValueMap *data,
                                     MapObject::IteratorKind kind);
    static void finalize(FreeOp *fop, JSObject *obj);

  private:
    static inline bool is(HandleValue v);
    inline ValueMap::Range *range();
    inline MapObject::IteratorKind kind() const;
    static bool next_impl(JSContext *cx, CallArgs args);
    static bool next(JSContext *cx, unsigned argc, Value *vp);
};

} 

const Class MapIteratorObject::class_ = {
    "Map Iterator",
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(MapIteratorObject::SlotCount),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    MapIteratorObject::finalize
};

const JSFunctionSpec MapIteratorObject::methods[] = {
    JS_SELF_HOSTED_FN("@@iterator", "IteratorIdentity", 0, 0),
    JS_FN("next", next, 0, 0),
    JS_FS_END
};

inline ValueMap::Range *
MapIteratorObject::range()
{
    return static_cast<ValueMap::Range *>(getSlot(RangeSlot).toPrivate());
}

inline MapObject::IteratorKind
MapIteratorObject::kind() const
{
    int32_t i = getSlot(KindSlot).toInt32();
    JS_ASSERT(i == MapObject::Keys || i == MapObject::Values || i == MapObject::Entries);
    return MapObject::IteratorKind(i);
}

bool
GlobalObject::initMapIteratorProto(JSContext *cx, Handle<GlobalObject *> global)
{
    JSObject *base = global->getOrCreateIteratorPrototype(cx);
    if (!base)
        return false;
    Rooted<JSObject*> proto(cx,
        NewObjectWithGivenProto(cx, &MapIteratorObject::class_, base, global));
    if (!proto)
        return false;
    proto->setSlot(MapIteratorObject::RangeSlot, PrivateValue(nullptr));
    if (!JS_DefineFunctions(cx, proto, MapIteratorObject::methods))
        return false;
    global->setReservedSlot(MAP_ITERATOR_PROTO, ObjectValue(*proto));
    return true;
}

MapIteratorObject *
MapIteratorObject::create(JSContext *cx, HandleObject mapobj, ValueMap *data,
                          MapObject::IteratorKind kind)
{
    Rooted<GlobalObject *> global(cx, &mapobj->global());
    Rooted<JSObject*> proto(cx, global->getOrCreateMapIteratorPrototype(cx));
    if (!proto)
        return nullptr;

    ValueMap::Range *range = cx->new_<ValueMap::Range>(data->all());
    if (!range)
        return nullptr;

    JSObject *iterobj = NewObjectWithGivenProto(cx, &class_, proto, global);
    if (!iterobj) {
        js_delete(range);
        return nullptr;
    }
    iterobj->setSlot(TargetSlot, ObjectValue(*mapobj));
    iterobj->setSlot(KindSlot, Int32Value(int32_t(kind)));
    iterobj->setSlot(RangeSlot, PrivateValue(range));
    return static_cast<MapIteratorObject *>(iterobj);
}

void
MapIteratorObject::finalize(FreeOp *fop, JSObject *obj)
{
    fop->delete_(obj->as<MapIteratorObject>().range());
}

bool
MapIteratorObject::is(HandleValue v)
{
    return v.isObject() && v.toObject().hasClass(&class_);
}

bool
MapIteratorObject::next_impl(JSContext *cx, CallArgs args)
{
    MapIteratorObject &thisobj = args.thisv().toObject().as<MapIteratorObject>();
    ValueMap::Range *range = thisobj.range();
    RootedValue value(cx);
    bool done;

    if (!range || range->empty()) {
        js_delete(range);
        thisobj.setReservedSlot(RangeSlot, PrivateValue(nullptr));
        value.setUndefined();
        done = true;
    } else {
        switch (thisobj.kind()) {
          case MapObject::Keys:
            value = range->front().key.get();
            break;

          case MapObject::Values:
            value = range->front().value;
            break;

          case MapObject::Entries: {
            Value pair[2] = { range->front().key.get(), range->front().value };
            AutoValueArray root(cx, pair, ArrayLength(pair));

            JSObject *pairobj = NewDenseCopiedArray(cx, ArrayLength(pair), pair);
            if (!pairobj)
                return false;
            value.setObject(*pairobj);
            break;
          }
        }
        range->popFront();
        done = false;
    }

    RootedObject result(cx, CreateItrResultObject(cx, value, done));
    if (!result)
        return false;
    args.rval().setObject(*result);

    return true;
}

bool
MapIteratorObject::next(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod(cx, is, next_impl, args);
}




const Class MapObject::class_ = {
    "Map",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Map),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    finalize,
    nullptr,                 
    nullptr,                 
    nullptr,                 
    nullptr,                 
    mark
};

const JSPropertySpec MapObject::properties[] = {
    JS_PSG("size", size, 0),
    JS_PS_END
};

const JSFunctionSpec MapObject::methods[] = {
    JS_FN("get", get, 1, 0),
    JS_FN("has", has, 1, 0),
    JS_FN("set", set, 2, 0),
    JS_FN("delete", delete_, 1, 0),
    JS_FN("keys", keys, 0, 0),
    JS_FN("values", values, 0, 0),
    JS_FN("clear", clear, 0, 0),
    JS_SELF_HOSTED_FN("forEach", "MapForEach", 2, 0),
    JS_FS_END
};

static JSObject *
InitClass(JSContext *cx, Handle<GlobalObject*> global, const Class *clasp, JSProtoKey key, Native construct,
          const JSPropertySpec *properties, const JSFunctionSpec *methods)
{
    Rooted<JSObject*> proto(cx, global->createBlankPrototype(cx, clasp));
    if (!proto)
        return nullptr;
    proto->setPrivate(nullptr);

    Rooted<JSFunction*> ctor(cx, global->createConstructor(cx, construct, ClassName(key, cx), 1));
    if (!ctor ||
        !LinkConstructorAndPrototype(cx, ctor, proto) ||
        !DefinePropertiesAndBrand(cx, proto, properties, methods) ||
        !DefineConstructorAndPrototype(cx, global, key, ctor, proto))
    {
        return nullptr;
    }
    return proto;
}

JSObject *
MapObject::initClass(JSContext *cx, JSObject *obj)
{
    Rooted<GlobalObject*> global(cx, &obj->as<GlobalObject>());
    RootedObject proto(cx,
        InitClass(cx, global, &class_, JSProto_Map, construct, properties, methods));
    if (proto) {
        
        JSFunction *fun = JS_DefineFunction(cx, proto, "entries", entries, 0, 0);
        if (!fun)
            return nullptr;

        
        RootedValue funval(cx, ObjectValue(*fun));
        if (!JS_DefineProperty(cx, proto, js_std_iterator_str, funval, nullptr, nullptr, 0))
            return nullptr;
    }
    return proto;
}

template <class Range>
static void
MarkKey(Range &r, const HashableValue &key, JSTracer *trc)
{
    HashableValue newKey = key.mark(trc);

    if (newKey.get() != key.get()) {
        
        
        r.rekeyFront(newKey);
    }
}

void
MapObject::mark(JSTracer *trc, JSObject *obj)
{
    if (ValueMap *map = obj->as<MapObject>().getData()) {
        for (ValueMap::Range r = map->all(); !r.empty(); r.popFront()) {
            MarkKey(r, r.front().key, trc);
            gc::MarkValue(trc, &r.front().value, "value");
        }
    }
}

#ifdef JSGC_GENERATIONAL
template <typename TableType>
class OrderedHashTableRef : public gc::BufferableRef
{
    TableType *table;
    HashableValue key;

  public:
    explicit OrderedHashTableRef(TableType *t, const HashableValue &k) : table(t), key(k) {}

    void mark(JSTracer *trc) {
        HashableValue prior = key;
        key = key.mark(trc);
        table->rekeyOneEntry(prior, key);
    }
};
#endif

template <typename TableType>
static void
WriteBarrierPost(JSRuntime *rt, TableType *table, const HashableValue &key)
{
#ifdef JSGC_GENERATIONAL
    rt->gcStoreBuffer.putGeneric(OrderedHashTableRef<TableType>(table, key));
#endif
}

void
MapObject::finalize(FreeOp *fop, JSObject *obj)
{
    if (ValueMap *map = obj->as<MapObject>().getData())
        fop->delete_(map);
}

bool
MapObject::construct(JSContext *cx, unsigned argc, Value *vp)
{
    Rooted<JSObject*> obj(cx, NewBuiltinClassInstance(cx, &class_));
    if (!obj)
        return false;

    ValueMap *map = cx->new_<ValueMap>(cx->runtime());
    if (!map)
        return false;
    if (!map->init()) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    obj->setPrivate(map);

    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.hasDefined(0)) {
        ForOfIterator iter(cx);
        if (!iter.init(args[0]))
            return false;
        RootedValue pairVal(cx);
        RootedObject pairObj(cx);
        while (true) {
            bool done;
            if (!iter.next(&pairVal, &done))
                return false;
            if (done)
                break;
            if (!pairVal.isObject()) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INVALID_MAP_ITERABLE);
                return false;
            }

            pairObj = &pairVal.toObject();
            if (!pairObj)
                return false;

            RootedValue key(cx);
            if (!JSObject::getElement(cx, pairObj, pairObj, 0, &key))
                return false;

            AutoHashableValueRooter hkey(cx);
            if (!hkey.setValue(cx, key))
                return false;

            RootedValue val(cx);
            if (!JSObject::getElement(cx, pairObj, pairObj, 1, &val))
                return false;

            RelocatableValue rval(val);
            if (!map->put(hkey, rval)) {
                js_ReportOutOfMemory(cx);
                return false;
            }
            WriteBarrierPost(cx->runtime(), map, hkey);
        }
    }

    args.rval().setObject(*obj);
    return true;
}

bool
MapObject::is(HandleValue v)
{
    return v.isObject() && v.toObject().hasClass(&class_) && v.toObject().getPrivate();
}

#define ARG0_KEY(cx, args, key)                                               \
    AutoHashableValueRooter key(cx);                                          \
    if (args.length() > 0 && !key.setValue(cx, args[0]))                      \
        return false

ValueMap &
MapObject::extract(CallReceiver call)
{
    JS_ASSERT(call.thisv().isObject());
    JS_ASSERT(call.thisv().toObject().hasClass(&MapObject::class_));
    return *call.thisv().toObject().as<MapObject>().getData();
}

bool
MapObject::size_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(MapObject::is(args.thisv()));

    ValueMap &map = extract(args);
    JS_STATIC_ASSERT(sizeof map.count() <= sizeof(uint32_t));
    args.rval().setNumber(map.count());
    return true;
}

bool
MapObject::size(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<MapObject::is, MapObject::size_impl>(cx, args);
}

bool
MapObject::get_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(MapObject::is(args.thisv()));

    ValueMap &map = extract(args);
    ARG0_KEY(cx, args, key);

    if (ValueMap::Entry *p = map.get(key))
        args.rval().set(p->value);
    else
        args.rval().setUndefined();
    return true;
}

bool
MapObject::get(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<MapObject::is, MapObject::get_impl>(cx, args);
}

bool
MapObject::has_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(MapObject::is(args.thisv()));

    ValueMap &map = extract(args);
    ARG0_KEY(cx, args, key);
    args.rval().setBoolean(map.has(key));
    return true;
}

bool
MapObject::has(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<MapObject::is, MapObject::has_impl>(cx, args);
}

bool
MapObject::set_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(MapObject::is(args.thisv()));

    ValueMap &map = extract(args);
    ARG0_KEY(cx, args, key);
    RelocatableValue rval(args.get(1));
    if (!map.put(key, rval)) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    WriteBarrierPost(cx->runtime(), &map, key);
    args.rval().setUndefined();
    return true;
}

bool
MapObject::set(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<MapObject::is, MapObject::set_impl>(cx, args);
}

bool
MapObject::delete_impl(JSContext *cx, CallArgs args)
{
    
    
    
    
    
    
    
    
    
    JS_ASSERT(MapObject::is(args.thisv()));

    ValueMap &map = extract(args);
    ARG0_KEY(cx, args, key);
    bool found;
    if (!map.remove(key, &found)) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    args.rval().setBoolean(found);
    return true;
}

bool
MapObject::delete_(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<MapObject::is, MapObject::delete_impl>(cx, args);
}

bool
MapObject::iterator_impl(JSContext *cx, CallArgs args, IteratorKind kind)
{
    Rooted<MapObject*> mapobj(cx, &args.thisv().toObject().as<MapObject>());
    ValueMap &map = *mapobj->getData();
    Rooted<JSObject*> iterobj(cx, MapIteratorObject::create(cx, mapobj, &map, kind));
    if (!iterobj)
        return false;
    args.rval().setObject(*iterobj);
    return true;
}

bool
MapObject::keys_impl(JSContext *cx, CallArgs args)
{
    return iterator_impl(cx, args, Keys);
}

bool
MapObject::keys(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod(cx, is, keys_impl, args);
}

bool
MapObject::values_impl(JSContext *cx, CallArgs args)
{
    return iterator_impl(cx, args, Values);
}

bool
MapObject::values(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod(cx, is, values_impl, args);
}

bool
MapObject::entries_impl(JSContext *cx, CallArgs args)
{
    return iterator_impl(cx, args, Entries);
}

bool
MapObject::entries(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod(cx, is, entries_impl, args);
}

bool
MapObject::clear_impl(JSContext *cx, CallArgs args)
{
    Rooted<MapObject*> mapobj(cx, &args.thisv().toObject().as<MapObject>());
    if (!mapobj->getData()->clear()) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    args.rval().setUndefined();
    return true;
}

bool
MapObject::clear(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod(cx, is, clear_impl, args);
}

JSObject *
js_InitMapClass(JSContext *cx, HandleObject obj)
{
    return MapObject::initClass(cx, obj);
}




namespace {

class SetIteratorObject : public JSObject
{
  public:
    static const Class class_;

    enum { TargetSlot, KindSlot, RangeSlot, SlotCount };
    static const JSFunctionSpec methods[];
    static SetIteratorObject *create(JSContext *cx, HandleObject setobj, ValueSet *data,
                                     SetObject::IteratorKind kind);
    static void finalize(FreeOp *fop, JSObject *obj);

  private:
    static inline bool is(HandleValue v);
    inline ValueSet::Range *range();
    inline SetObject::IteratorKind kind() const;
    static bool next_impl(JSContext *cx, CallArgs args);
    static bool next(JSContext *cx, unsigned argc, Value *vp);
};

} 

const Class SetIteratorObject::class_ = {
    "Set Iterator",
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(SetIteratorObject::SlotCount),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    SetIteratorObject::finalize
};

const JSFunctionSpec SetIteratorObject::methods[] = {
    JS_SELF_HOSTED_FN("@@iterator", "IteratorIdentity", 0, 0),
    JS_FN("next", next, 0, 0),
    JS_FS_END
};

inline ValueSet::Range *
SetIteratorObject::range()
{
    return static_cast<ValueSet::Range *>(getSlot(RangeSlot).toPrivate());
}

inline SetObject::IteratorKind
SetIteratorObject::kind() const
{
    int32_t i = getSlot(KindSlot).toInt32();
    JS_ASSERT(i == SetObject::Values || i == SetObject::Entries);
    return SetObject::IteratorKind(i);
}

bool
GlobalObject::initSetIteratorProto(JSContext *cx, Handle<GlobalObject*> global)
{
    JSObject *base = global->getOrCreateIteratorPrototype(cx);
    if (!base)
        return false;
    RootedObject proto(cx, NewObjectWithGivenProto(cx, &SetIteratorObject::class_, base, global));
    if (!proto)
        return false;
    proto->setSlot(SetIteratorObject::RangeSlot, PrivateValue(nullptr));
    if (!JS_DefineFunctions(cx, proto, SetIteratorObject::methods))
        return false;
    global->setReservedSlot(SET_ITERATOR_PROTO, ObjectValue(*proto));
    return true;
}

SetIteratorObject *
SetIteratorObject::create(JSContext *cx, HandleObject setobj, ValueSet *data,
                          SetObject::IteratorKind kind)
{
    Rooted<GlobalObject *> global(cx, &setobj->global());
    Rooted<JSObject*> proto(cx, global->getOrCreateSetIteratorPrototype(cx));
    if (!proto)
        return nullptr;

    ValueSet::Range *range = cx->new_<ValueSet::Range>(data->all());
    if (!range)
        return nullptr;

    JSObject *iterobj = NewObjectWithGivenProto(cx, &class_, proto, global);
    if (!iterobj) {
        js_delete(range);
        return nullptr;
    }
    iterobj->setSlot(TargetSlot, ObjectValue(*setobj));
    iterobj->setSlot(KindSlot, Int32Value(int32_t(kind)));
    iterobj->setSlot(RangeSlot, PrivateValue(range));
    return static_cast<SetIteratorObject *>(iterobj);
}

void
SetIteratorObject::finalize(FreeOp *fop, JSObject *obj)
{
    fop->delete_(obj->as<SetIteratorObject>().range());
}

bool
SetIteratorObject::is(HandleValue v)
{
    return v.isObject() && v.toObject().is<SetIteratorObject>();
}

bool
SetIteratorObject::next_impl(JSContext *cx, CallArgs args)
{
    SetIteratorObject &thisobj = args.thisv().toObject().as<SetIteratorObject>();
    ValueSet::Range *range = thisobj.range();
    RootedValue value(cx);
    bool done;

    if (!range || range->empty()) {
        js_delete(range);
        thisobj.setReservedSlot(RangeSlot, PrivateValue(nullptr));
        value.setUndefined();
        done = true;
    } else {
        switch (thisobj.kind()) {
          case SetObject::Values:
            value = range->front().get();
            break;

          case SetObject::Entries: {
            Value pair[2] = { range->front().get(), range->front().get() };
            AutoValueArray root(cx, pair, 2);

            JSObject *pairObj = NewDenseCopiedArray(cx, 2, pair);
            if (!pairObj)
              return false;
            value.setObject(*pairObj);
            break;
          }
        }
        range->popFront();
        done = false;
    }

    RootedObject result(cx, CreateItrResultObject(cx, value, done));
    if (!result)
        return false;
    args.rval().setObject(*result);

    return true;
}

bool
SetIteratorObject::next(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod(cx, is, next_impl, args);
}




const Class SetObject::class_ = {
    "Set",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Set),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    finalize,
    nullptr,                 
    nullptr,                 
    nullptr,                 
    nullptr,                 
    mark
};

const JSPropertySpec SetObject::properties[] = {
    JS_PSG("size", size, 0),
    JS_PS_END
};

const JSFunctionSpec SetObject::methods[] = {
    JS_FN("has", has, 1, 0),
    JS_FN("add", add, 1, 0),
    JS_FN("delete", delete_, 1, 0),
    JS_FN("entries", entries, 0, 0),
    JS_FN("clear", clear, 0, 0),
    JS_SELF_HOSTED_FN("forEach", "SetForEach", 2, 0),
    JS_FS_END
};

JSObject *
SetObject::initClass(JSContext *cx, JSObject *obj)
{
    Rooted<GlobalObject*> global(cx, &obj->as<GlobalObject>());
    RootedObject proto(cx,
        InitClass(cx, global, &class_, JSProto_Set, construct, properties, methods));
    if (proto) {
        
        JSFunction *fun = JS_DefineFunction(cx, proto, "values", values, 0, 0);
        if (!fun)
            return nullptr;

        
        RootedValue funval(cx, ObjectValue(*fun));
        if (!JS_DefineProperty(cx, proto, "keys", funval, nullptr, nullptr, 0))
            return nullptr;
        if (!JS_DefineProperty(cx, proto, js_std_iterator_str, funval, nullptr, nullptr, 0))
            return nullptr;
    }
    return proto;
}

void
SetObject::mark(JSTracer *trc, JSObject *obj)
{
    SetObject *setobj = static_cast<SetObject *>(obj);
    if (ValueSet *set = setobj->getData()) {
        for (ValueSet::Range r = set->all(); !r.empty(); r.popFront())
            MarkKey(r, r.front(), trc);
    }
}

void
SetObject::finalize(FreeOp *fop, JSObject *obj)
{
    SetObject *setobj = static_cast<SetObject *>(obj);
    if (ValueSet *set = setobj->getData())
        fop->delete_(set);
}

bool
SetObject::construct(JSContext *cx, unsigned argc, Value *vp)
{
    Rooted<JSObject*> obj(cx, NewBuiltinClassInstance(cx, &class_));
    if (!obj)
        return false;

    ValueSet *set = cx->new_<ValueSet>(cx->runtime());
    if (!set)
        return false;
    if (!set->init()) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    obj->setPrivate(set);

    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.hasDefined(0)) {
        RootedValue keyVal(cx);
        ForOfIterator iter(cx);
        if (!iter.init(args[0]))
            return false;
        AutoHashableValueRooter key(cx);
        while (true) {
            bool done;
            if (!iter.next(&keyVal, &done))
                return false;
            if (done)
                break;
            if (!key.setValue(cx, keyVal))
                return false;
            if (!set->put(key)) {
                js_ReportOutOfMemory(cx);
                return false;
            }
            WriteBarrierPost(cx->runtime(), set, key);
        }
    }

    args.rval().setObject(*obj);
    return true;
}

bool
SetObject::is(HandleValue v)
{
    return v.isObject() && v.toObject().hasClass(&class_) && v.toObject().getPrivate();
}

ValueSet &
SetObject::extract(CallReceiver call)
{
    JS_ASSERT(call.thisv().isObject());
    JS_ASSERT(call.thisv().toObject().hasClass(&SetObject::class_));
    return *static_cast<SetObject&>(call.thisv().toObject()).getData();
}

bool
SetObject::size_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(is(args.thisv()));

    ValueSet &set = extract(args);
    JS_STATIC_ASSERT(sizeof set.count() <= sizeof(uint32_t));
    args.rval().setNumber(set.count());
    return true;
}

bool
SetObject::size(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<SetObject::is, SetObject::size_impl>(cx, args);
}

bool
SetObject::has_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(is(args.thisv()));

    ValueSet &set = extract(args);
    ARG0_KEY(cx, args, key);
    args.rval().setBoolean(set.has(key));
    return true;
}

bool
SetObject::has(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<SetObject::is, SetObject::has_impl>(cx, args);
}

bool
SetObject::add_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(is(args.thisv()));

    ValueSet &set = extract(args);
    ARG0_KEY(cx, args, key);
    if (!set.put(key)) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    WriteBarrierPost(cx->runtime(), &set, key);
    args.rval().setUndefined();
    return true;
}

bool
SetObject::add(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<SetObject::is, SetObject::add_impl>(cx, args);
}

bool
SetObject::delete_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(is(args.thisv()));

    ValueSet &set = extract(args);
    ARG0_KEY(cx, args, key);
    bool found;
    if (!set.remove(key, &found)) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    args.rval().setBoolean(found);
    return true;
}

bool
SetObject::delete_(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<SetObject::is, SetObject::delete_impl>(cx, args);
}

bool
SetObject::iterator_impl(JSContext *cx, CallArgs args, IteratorKind kind)
{
    Rooted<SetObject*> setobj(cx, &args.thisv().toObject().as<SetObject>());
    ValueSet &set = *setobj->getData();
    Rooted<JSObject*> iterobj(cx, SetIteratorObject::create(cx, setobj, &set, kind));
    if (!iterobj)
        return false;
    args.rval().setObject(*iterobj);
    return true;
}

bool
SetObject::values_impl(JSContext *cx, CallArgs args)
{
    return iterator_impl(cx, args, Values);
}

bool
SetObject::values(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod(cx, is, values_impl, args);
}

bool
SetObject::entries_impl(JSContext *cx, CallArgs args)
{
    return iterator_impl(cx, args, Entries);
}

bool
SetObject::entries(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod(cx, is, entries_impl, args);
}

bool
SetObject::clear_impl(JSContext *cx, CallArgs args)
{
    Rooted<SetObject*> setobj(cx, &args.thisv().toObject().as<SetObject>());
    if (!setobj->getData()->clear()) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    args.rval().setUndefined();
    return true;
}

bool
SetObject::clear(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod(cx, is, clear_impl, args);
}

JSObject *
js_InitSetClass(JSContext *cx, HandleObject obj)
{
    return SetObject::initClass(cx, obj);
}
