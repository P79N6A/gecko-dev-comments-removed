






#include "builtin/MapObject.h"

#include "jscntxt.h"
#include "jsiter.h"
#include "jsobj.h"

#include "gc/Marking.h"
#include "js/Utility.h"
#include "vm/GlobalObject.h"
#include "vm/Stack.h"

#include "jsobjinlines.h"

using namespace js;
































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
        : hashTable(NULL), data(NULL), dataLength(0), ranges(NULL), alloc(ap) {}

    bool init() {
        MOZ_ASSERT(!hashTable, "init must be called at most once");

        uint32_t buckets = initialBuckets();
        Data **tableAlloc = static_cast<Data **>(alloc.malloc_(buckets * sizeof(Data *)));
        if (!tableAlloc)
            return false;
        for (uint32_t i = 0; i < buckets; i++)
            tableAlloc[i] = NULL;

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
        return lookup(l) != NULL;
    }

    
    T *get(const Lookup &l) {
        Data *e = lookup(l, prepareHash(l));
        return e ? &e->element : NULL;
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
        if (e == NULL) {
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

    















    enum CallDestructors {
        DoNotCallDtors = false,
        DoCallDtors = true
    };
    bool clear(CallDestructors callDestructors = DoCallDtors) {
        if (dataLength != 0) {
            Data **oldHashTable = hashTable;
            Data *oldData = data;
            uint32_t oldDataLength = dataLength;

            hashTable = NULL;
            if (!init()) {
                
                hashTable = oldHashTable;
                return false;
            }

            alloc.free_(oldHashTable);
            if (callDestructors)
                destroyData(oldData, oldDataLength);
            alloc.free_(oldData);
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

        




        void rekeyFrontWithSameHashCode(const Key &k) {
            MOZ_ASSERT(valid());
#ifdef DEBUG
            
            HashNumber h = Ops::hash(k) >> ht.hashShift;
            Data *e = ht.hashTable[h];
            while (e && e != &ht.data[i])
                e = e->chain;
            MOZ_ASSERT(e == &ht.data[i]);
#endif
            Ops::setKey(ht.data[i].element, k);
        }
    };

    Range all() { return Range(*this); }

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
        return NULL;
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
            hashTable[i] = NULL;
        Data *wp = data, *end = data + dataLength;
        for (Data *rp = data; rp != end; rp++) {
            if (!Ops::isEmpty(Ops::getKey(rp->element))) {
                HashNumber h = prepareHash(Ops::getKey(rp->element)) >> hashShift;
                if (rp != wp)
                    wp->element = Move(rp->element);
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
            newHashTable[i] = NULL;

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
                new (wp) Data(Move(p->element), newHashTable[h]);
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
            const_cast<Key &>(key) = Move(rhs->key);
            value = Move(rhs->value);
        }

      public:
        Entry() : key(), value() {}
        Entry(const Key &k, const Value &v) : key(k), value(v) {}
        Entry(MoveRef<Entry> rhs) : key(Move(rhs->key)), value(Move(rhs->value)) {}

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
    bool clear()                                    { return impl.clear(Impl::DoCallDtors); }
    bool clearWithoutCallingDestructors()           { return impl.clear(Impl::DoNotCallDtors); }
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
    bool clear()                                    { return impl.clear(Impl::DoCallDtors); }
    bool clearWithoutCallingDestructors()           { return impl.clear(Impl::DoNotCallDtors); }
};

}  




bool
HashableValue::setValue(JSContext *cx, const Value &v)
{
    if (v.isString()) {
        
        JSString *str = AtomizeString<CanGC>(cx, v.toString(), DoNotInternAtom);
        if (!str)
            return false;
        value = StringValue(str);
    } else if (v.isDouble()) {
        double d = v.toDouble();
        int32_t i;
        if (MOZ_DOUBLE_IS_INT32(d, &i)) {
            
            value = Int32Value(i);
        } else if (MOZ_DOUBLE_IS_NaN(d)) {
            
            value = DoubleValue(js_NaN);
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
HashableValue::equals(const HashableValue &other) const
{
    
    bool b = (value.asRawBits() == other.value.asRawBits());

#ifdef DEBUG
    bool same;
    JS_ASSERT(SameValue(NULL, value, other.value, &same));
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




class js::MapIteratorObject : public JSObject
{
  public:
    enum { TargetSlot, KindSlot, RangeSlot, SlotCount };
    static JSFunctionSpec methods[];
    static MapIteratorObject *create(JSContext *cx, HandleObject mapobj, ValueMap *data,
                                     MapObject::IteratorKind kind);
    static void finalize(FreeOp *fop, RawObject obj);

  private:
    static inline bool is(const Value &v);
    inline ValueMap::Range *range();
    inline MapObject::IteratorKind kind() const;
    static bool next_impl(JSContext *cx, CallArgs args);
    static JSBool next(JSContext *cx, unsigned argc, Value *vp);
};

inline js::MapIteratorObject &
JSObject::asMapIterator()
{
    JS_ASSERT(isMapIterator());
    return *static_cast<js::MapIteratorObject *>(this);
}

Class js::MapIteratorClass = {
    "Map Iterator",
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(MapIteratorObject::SlotCount),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    MapIteratorObject::finalize
};

JSFunctionSpec MapIteratorObject::methods[] = {
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
        NewObjectWithGivenProto(cx, &MapIteratorClass, base, global));
    if (!proto)
        return false;
    proto->setSlot(MapIteratorObject::RangeSlot, PrivateValue(NULL));
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
        return NULL;

    ValueMap::Range *range = cx->new_<ValueMap::Range>(data->all());
    if (!range)
        return NULL;

    JSObject *iterobj = NewObjectWithGivenProto(cx, &MapIteratorClass, proto, global);
    if (!iterobj) {
        js_delete(range);
        return NULL;
    }
    iterobj->setSlot(TargetSlot, ObjectValue(*mapobj));
    iterobj->setSlot(KindSlot, Int32Value(int32_t(kind)));
    iterobj->setSlot(RangeSlot, PrivateValue(range));
    return static_cast<MapIteratorObject *>(iterobj);
}

void
MapIteratorObject::finalize(FreeOp *fop, RawObject obj)
{
    fop->delete_(obj->asMapIterator().range());
}

bool
MapIteratorObject::is(const Value &v)
{
    return v.isObject() && v.toObject().hasClass(&MapIteratorClass);
}

bool
MapIteratorObject::next_impl(JSContext *cx, CallArgs args)
{
    MapIteratorObject &thisobj = args.thisv().toObject().asMapIterator();
    ValueMap::Range *range = thisobj.range();
    if (!range)
        return js_ThrowStopIteration(cx);
    if (range->empty()) {
        js_delete(range);
        thisobj.setReservedSlot(RangeSlot, PrivateValue(NULL));
        return js_ThrowStopIteration(cx);
    }

    switch (thisobj.kind()) {
      case MapObject::Keys:
        args.rval().set(range->front().key.get());
        break;

      case MapObject::Values:
        args.rval().set(range->front().value);
        break;

      case MapObject::Entries: {
        Value pair[2] = { range->front().key.get(), range->front().value };
        AutoValueArray root(cx, pair, 2);

        JSObject *pairobj = NewDenseCopiedArray(cx, 2, pair);
        if (!pairobj)
            return false;
        args.rval().setObject(*pairobj);
        break;
      }
    }
    range->popFront();
    return true;
}

JSBool
MapIteratorObject::next(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod(cx, is, next_impl, args);
}




inline js::MapObject &
JSObject::asMap()
{
    JS_ASSERT(hasClass(&MapObject::class_));
    return *static_cast<js::MapObject *>(this);
}

Class MapObject::class_ = {
    "Map",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Map),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    finalize,
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    mark
};

JSPropertySpec MapObject::properties[] = {
    JS_PSG("size", size, 0),
    JS_PS_END
};

JSFunctionSpec MapObject::methods[] = {
    JS_FN("get", get, 1, 0),
    JS_FN("has", has, 1, 0),
    JS_FN("set", set, 2, 0),
    JS_FN("delete", delete_, 1, 0),
    JS_FN("keys", keys, 0, 0),
    JS_FN("values", values, 0, 0),
    JS_FN("entries", entries, 0, 0),
    JS_FN("iterator", entries, 0, 0),
    JS_FN("clear", clear, 0, 0),
    JS_FS_END
};

static JSObject *
InitClass(JSContext *cx, Handle<GlobalObject*> global, Class *clasp, JSProtoKey key, Native construct,
          JSPropertySpec *properties, JSFunctionSpec *methods)
{
    Rooted<JSObject*> proto(cx, global->createBlankPrototype(cx, clasp));
    if (!proto)
        return NULL;
    proto->setPrivate(NULL);

    Rooted<JSFunction*> ctor(cx, global->createConstructor(cx, construct, ClassName(key, cx), 1));
    if (!ctor ||
        !LinkConstructorAndPrototype(cx, ctor, proto) ||
        !DefinePropertiesAndBrand(cx, proto, properties, methods) ||
        !DefineConstructorAndPrototype(cx, global, key, ctor, proto))
    {
        return NULL;
    }
    return proto;
}

JSObject *
MapObject::initClass(JSContext *cx, JSObject *obj)
{
    Rooted<GlobalObject*> global(cx, &obj->asGlobal());
    return InitClass(cx, global, &class_, JSProto_Map, construct, properties, methods);
}

template <class Range>
static void
MarkKey(Range &r, const HashableValue &key, JSTracer *trc)
{
    HashableValue newKey = key.mark(trc);

    if (newKey.get() != key.get()) {
        if (newKey.get().isString()) {
            
            
            
            
            
            
            
            r.rekeyFrontWithSameHashCode(newKey);
        } else {
            
            
            

            JS_ASSERT(newKey.get().isObject());
            r.rekeyFront(newKey);
        }
    }
}

void
MapObject::mark(JSTracer *trc, RawObject obj)
{
    if (ValueMap *map = obj->asMap().getData()) {
        for (ValueMap::Range r = map->all(); !r.empty(); r.popFront()) {
            MarkKey(r, r.front().key, trc);
            gc::MarkValue(trc, &r.front().value, "value");
        }
    }
}

void
MapObject::finalize(FreeOp *fop, RawObject obj)
{
    if (ValueMap *map = obj->asMap().getData()) {
        map->clearWithoutCallingDestructors();
        fop->delete_(map);
    }
}

JSBool
MapObject::construct(JSContext *cx, unsigned argc, Value *vp)
{
    Rooted<JSObject*> obj(cx, NewBuiltinClassInstance(cx, &class_));
    if (!obj)
        return false;

    ValueMap *map = cx->new_<ValueMap>(cx->runtime);
    if (!map)
        return false;
    if (!map->init()) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    obj->setPrivate(map);

    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.hasDefined(0)) {
        ForOfIterator iter(cx, args[0]);
        while (iter.next()) {
            RootedObject pairobj(cx, js_ValueToNonNullObject(cx, iter.value()));
            if (!pairobj)
                return false;

            RootedValue key(cx);
            if (!JSObject::getElement(cx, pairobj, pairobj, 0, &key))
                return false;

            HashableValue hkey;
            HashableValue::AutoRooter hkeyRoot(cx, &hkey);
            if (!hkey.setValue(cx, key))
                return false;

            RootedValue val(cx);
            if (!JSObject::getElement(cx, pairobj, pairobj, 1, &val))
                return false;

            RelocatableValue rval(val);
            if (!map->put(hkey, rval)) {
                js_ReportOutOfMemory(cx);
                return false;
            }
        }
        if (!iter.close())
            return false;
    }

    args.rval().setObject(*obj);
    return true;
}

bool
MapObject::is(const Value &v)
{
    return v.isObject() && v.toObject().hasClass(&class_) && v.toObject().getPrivate();
}

#define ARG0_KEY(cx, args, key)                                               \
    HashableValue key;                                                        \
    HashableValue::AutoRooter keyRoot(cx, &key);                              \
    if (args.length() > 0 && !key.setValue(cx, args[0]))                      \
        return false

ValueMap &
MapObject::extract(CallReceiver call)
{
    JS_ASSERT(call.thisv().isObject());
    JS_ASSERT(call.thisv().toObject().hasClass(&MapObject::class_));
    return *call.thisv().toObject().asMap().getData();
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

JSBool
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

JSBool
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

JSBool
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
    RelocatableValue rval(args.length() > 1 ? args[1] : UndefinedValue());
    if (!map.put(key, rval)) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    args.rval().setUndefined();
    return true;
}

JSBool
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

JSBool
MapObject::delete_(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<MapObject::is, MapObject::delete_impl>(cx, args);
}

bool
MapObject::iterator_impl(JSContext *cx, CallArgs args, IteratorKind kind)
{
    Rooted<MapObject*> mapobj(cx, &args.thisv().toObject().asMap());
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

JSBool
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

JSBool
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

JSBool
MapObject::entries(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod(cx, is, entries_impl, args);
}

bool
MapObject::clear_impl(JSContext *cx, CallArgs args)
{
    Rooted<MapObject*> mapobj(cx, &args.thisv().toObject().asMap());
    if (!mapobj->getData()->clear()) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    args.rval().setUndefined();
    return true;
}

JSBool
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




class js::SetIteratorObject : public JSObject
{
  public:
    enum { TargetSlot, RangeSlot, SlotCount };
    static JSFunctionSpec methods[];
    static SetIteratorObject *create(JSContext *cx, HandleObject setobj, ValueSet *data);
    static void finalize(FreeOp *fop, RawObject obj);

  private:
    static inline bool is(const Value &v);
    inline ValueSet::Range *range();
    static bool next_impl(JSContext *cx, CallArgs args);
    static JSBool next(JSContext *cx, unsigned argc, Value *vp);
};

inline js::SetIteratorObject &
JSObject::asSetIterator()
{
    JS_ASSERT(isSetIterator());
    return *static_cast<js::SetIteratorObject *>(this);
}

Class js::SetIteratorClass = {
    "Set Iterator",
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(SetIteratorObject::SlotCount),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    SetIteratorObject::finalize
};

JSFunctionSpec SetIteratorObject::methods[] = {
    JS_FN("next", next, 0, 0),
    JS_FS_END
};

inline ValueSet::Range *
SetIteratorObject::range()
{
    return static_cast<ValueSet::Range *>(getSlot(RangeSlot).toPrivate());
}

bool
GlobalObject::initSetIteratorProto(JSContext *cx, Handle<GlobalObject*> global)
{
    JSObject *base = global->getOrCreateIteratorPrototype(cx);
    if (!base)
        return false;
    RootedObject proto(cx, NewObjectWithGivenProto(cx, &SetIteratorClass, base, global));
    if (!proto)
        return false;
    proto->setSlot(SetIteratorObject::RangeSlot, PrivateValue(NULL));
    if (!JS_DefineFunctions(cx, proto, SetIteratorObject::methods))
        return false;
    global->setReservedSlot(SET_ITERATOR_PROTO, ObjectValue(*proto));
    return true;
}

SetIteratorObject *
SetIteratorObject::create(JSContext *cx, HandleObject setobj, ValueSet *data)
{
    Rooted<GlobalObject *> global(cx, &setobj->global());
    Rooted<JSObject*> proto(cx, global->getOrCreateSetIteratorPrototype(cx));
    if (!proto)
        return NULL;

    ValueSet::Range *range = cx->new_<ValueSet::Range>(data->all());
    if (!range)
        return NULL;

    JSObject *iterobj = NewObjectWithGivenProto(cx, &SetIteratorClass, proto, global);
    if (!iterobj) {
        js_delete(range);
        return NULL;
    }
    iterobj->setSlot(TargetSlot, ObjectValue(*setobj));
    iterobj->setSlot(RangeSlot, PrivateValue(range));
    return static_cast<SetIteratorObject *>(iterobj);
}

void
SetIteratorObject::finalize(FreeOp *fop, RawObject obj)
{
    fop->delete_(obj->asSetIterator().range());
}

bool
SetIteratorObject::is(const Value &v)
{
    return v.isObject() && v.toObject().hasClass(&SetIteratorClass);
}

bool
SetIteratorObject::next_impl(JSContext *cx, CallArgs args)
{
    SetIteratorObject &thisobj = args.thisv().toObject().asSetIterator();
    ValueSet::Range *range = thisobj.range();
    if (!range)
        return js_ThrowStopIteration(cx);
    if (range->empty()) {
        js_delete(range);
        thisobj.setReservedSlot(RangeSlot, PrivateValue(NULL));
        return js_ThrowStopIteration(cx);
    }

    args.rval().set(range->front().get());
    range->popFront();
    return true;
}

JSBool
SetIteratorObject::next(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod(cx, is, next_impl, args);
}




inline js::SetObject &
JSObject::asSet()
{
    JS_ASSERT(hasClass(&SetObject::class_));
    return *static_cast<js::SetObject *>(this);
}

Class SetObject::class_ = {
    "Set",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Set),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    finalize,
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    mark
};

JSPropertySpec SetObject::properties[] = {
    JS_PSG("size", size, 0),
    JS_PS_END
};

JSFunctionSpec SetObject::methods[] = {
    JS_FN("has", has, 1, 0),
    JS_FN("add", add, 1, 0),
    JS_FN("delete", delete_, 1, 0),
    JS_FN("iterator", iterator, 0, 0),
    JS_FN("clear", clear, 0, 0),
    JS_FS_END
};

JSObject *
SetObject::initClass(JSContext *cx, JSObject *obj)
{
    Rooted<GlobalObject*> global(cx, &obj->asGlobal());
    return InitClass(cx, global, &class_, JSProto_Set, construct, properties, methods);
}

void
SetObject::mark(JSTracer *trc, RawObject obj)
{
    SetObject *setobj = static_cast<SetObject *>(obj);
    if (ValueSet *set = setobj->getData()) {
        for (ValueSet::Range r = set->all(); !r.empty(); r.popFront())
            MarkKey(r, r.front(), trc);
    }
}

void
SetObject::finalize(FreeOp *fop, RawObject obj)
{
    SetObject *setobj = static_cast<SetObject *>(obj);
    if (ValueSet *set = setobj->getData()) {
        set->clearWithoutCallingDestructors();
        fop->delete_(set);
    }
}

JSBool
SetObject::construct(JSContext *cx, unsigned argc, Value *vp)
{
    Rooted<JSObject*> obj(cx, NewBuiltinClassInstance(cx, &class_));
    if (!obj)
        return false;

    ValueSet *set = cx->new_<ValueSet>(cx->runtime);
    if (!set)
        return false;
    if (!set->init()) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    obj->setPrivate(set);

    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.hasDefined(0)) {
        ForOfIterator iter(cx, args[0]);
        while (iter.next()) {
            HashableValue key;
            HashableValue::AutoRooter hkeyRoot(cx, &key);
            if (!key.setValue(cx, iter.value()))
                return false;
            if (!set->put(key)) {
                js_ReportOutOfMemory(cx);
                return false;
            }
        }
        if (!iter.close())
            return false;
    }

    args.rval().setObject(*obj);
    return true;
}

bool
SetObject::is(const Value &v)
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

JSBool
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

JSBool
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
    args.rval().setUndefined();
    return true;
}

JSBool
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

JSBool
SetObject::delete_(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<SetObject::is, SetObject::delete_impl>(cx, args);
}

bool
SetObject::iterator_impl(JSContext *cx, CallArgs args)
{
    Rooted<SetObject*> setobj(cx, &args.thisv().toObject().asSet());
    ValueSet &set = *setobj->getData();
    Rooted<JSObject*> iterobj(cx, SetIteratorObject::create(cx, setobj, &set));
    if (!iterobj)
        return false;
    args.rval().setObject(*iterobj);
    return true;
}

JSBool
SetObject::iterator(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod(cx, is, iterator_impl, args);
}

bool
SetObject::clear_impl(JSContext *cx, CallArgs args)
{
    Rooted<SetObject*> setobj(cx, &args.thisv().toObject().asSet());
    if (!setobj->getData()->clear()) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    args.rval().setUndefined();
    return true;
}

JSBool
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
