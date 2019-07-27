





#ifndef jsweakmap_h
#define jsweakmap_h

#include "jscompartment.h"
#include "jsfriendapi.h"
#include "jsobj.h"

#include "gc/Marking.h"
#include "gc/StoreBuffer.h"
#include "js/HashTable.h"

namespace js {
















static WeakMapBase * const WeakMapNotInList = reinterpret_cast<WeakMapBase*>(1);

typedef HashSet<WeakMapBase*, DefaultHasher<WeakMapBase*>, SystemAllocPolicy> WeakMapSet;



class WeakMapBase {
  public:
    WeakMapBase(JSObject* memOf, JSCompartment* c);
    virtual ~WeakMapBase();

    void trace(JSTracer* tracer);

    

    
    static void unmarkCompartment(JSCompartment* c);

    
    static void markAll(JSCompartment* c, JSTracer* tracer);

    
    
    
    
    static bool markCompartmentIteratively(JSCompartment* c, JSTracer* tracer);

    
    static bool findZoneEdgesForCompartment(JSCompartment* c);

    
    
    static void sweepCompartment(JSCompartment* c);

    
    static void traceAllMappings(WeakMapTracer* tracer);

    bool isInList() { return next != WeakMapNotInList; }

    
    static bool saveCompartmentMarkedWeakMaps(JSCompartment* c, WeakMapSet& markedWeakMaps);

    
    static void restoreCompartmentMarkedWeakMaps(WeakMapSet& markedWeakMaps);

    
    static void removeWeakMapFromList(WeakMapBase* weakmap);

  protected:
    
    
    virtual void nonMarkingTraceKeys(JSTracer* tracer) = 0;
    virtual void nonMarkingTraceValues(JSTracer* tracer) = 0;
    virtual bool markIteratively(JSTracer* tracer) = 0;
    virtual bool findZoneEdges() = 0;
    virtual void sweep() = 0;
    virtual void traceMappings(WeakMapTracer* tracer) = 0;
    virtual void finish() = 0;

    
    RelocatablePtrObject memberOf;

    
    JSCompartment* compartment;

    
    
    
    WeakMapBase* next;

    
    bool marked;
};

template <class Key, class Value,
          class HashPolicy = DefaultHasher<Key> >
class WeakMap : public HashMap<Key, Value, HashPolicy, RuntimeAllocPolicy>, public WeakMapBase
{
  public:
    typedef HashMap<Key, Value, HashPolicy, RuntimeAllocPolicy> Base;
    typedef typename Base::Enum Enum;
    typedef typename Base::Lookup Lookup;
    typedef typename Base::Range Range;
    typedef typename Base::Ptr Ptr;
    typedef typename Base::AddPtr AddPtr;

    explicit WeakMap(JSContext* cx, JSObject* memOf = nullptr)
        : Base(cx->runtime()), WeakMapBase(memOf, cx->compartment()) { }

    bool init(uint32_t len = 16) {
        if (!Base::init(len))
            return false;
        next = compartment->gcWeakMapList;
        compartment->gcWeakMapList = this;
        marked = JS::IsIncrementalGCInProgress(compartment->runtimeFromMainThread());
        return true;
    }

    
    
    
    Ptr lookup(const Lookup& l) const {
        Ptr p = Base::lookup(l);
        if (p)
            exposeGCThingToActiveJS(p->value());
        return p;
    }

    AddPtr lookupForAdd(const Lookup& l) const {
        AddPtr p = Base::lookupForAdd(l);
        if (p)
            exposeGCThingToActiveJS(p->value());
        return p;
    }

    Ptr lookupWithDefault(const Key& k, const Value& defaultValue) {
        Ptr p = Base::lookupWithDefault(k, defaultValue);
        if (p)
            exposeGCThingToActiveJS(p->value());
        return p;
    }

  private:
    void exposeGCThingToActiveJS(const JS::Value& v) const { JS::ExposeValueToActiveJS(v); }
    void exposeGCThingToActiveJS(JSObject* obj) const { JS::ExposeObjectToActiveJS(obj); }

    bool markValue(JSTracer* trc, Value* x) {
        if (gc::IsMarked(x))
            return false;
        TraceEdge(trc, x, "WeakMap entry value");
        MOZ_ASSERT(gc::IsMarked(x));
        return true;
    }

    void nonMarkingTraceKeys(JSTracer* trc) {
        for (Enum e(*this); !e.empty(); e.popFront()) {
            Key key(e.front().key());
            TraceEdge(trc, &key, "WeakMap entry key");
            if (key != e.front().key())
                entryMoved(e, key);
        }
    }

    void nonMarkingTraceValues(JSTracer* trc) {
        for (Range r = Base::all(); !r.empty(); r.popFront())
            TraceEdge(trc, &r.front().value(), "WeakMap entry value");
    }

    bool keyNeedsMark(JSObject* key) {
        if (JSWeakmapKeyDelegateOp op = key->getClass()->ext.weakmapKeyDelegateOp) {
            JSObject* delegate = op(key);
            




            return delegate && gc::IsMarkedUnbarriered(&delegate);
        }
        return false;
    }

    bool keyNeedsMark(gc::Cell* cell) {
        return false;
    }

    bool markIteratively(JSTracer* trc) {
        bool markedAny = false;
        for (Enum e(*this); !e.empty(); e.popFront()) {
            
            Key key(e.front().key());
            if (gc::IsMarked(const_cast<Key*>(&key))) {
                if (markValue(trc, &e.front().value()))
                    markedAny = true;
                if (e.front().key() != key)
                    entryMoved(e, key);
            } else if (keyNeedsMark(key)) {
                TraceEdge(trc, &e.front().value(), "WeakMap entry value");
                TraceEdge(trc, &key, "proxy-preserved WeakMap entry key");
                if (e.front().key() != key)
                    entryMoved(e, key);
                markedAny = true;
            }
            key.unsafeSet(nullptr);
        }
        return markedAny;
    }

    bool findZoneEdges() {
        
        return true;
    }

    void sweep() {
        
        for (Enum e(*this); !e.empty(); e.popFront()) {
            Key k(e.front().key());
            if (gc::IsAboutToBeFinalized(&k))
                e.removeFront();
            else if (k != e.front().key())
                entryMoved(e, k);
        }
        



        assertEntriesNotAboutToBeFinalized();
    }

    void finish() {
        Base::finish();
    }

    
    void traceMappings(WeakMapTracer* tracer) {
        for (Range r = Base::all(); !r.empty(); r.popFront()) {
            gc::Cell* key = gc::ToMarkable(r.front().key());
            gc::Cell* value = gc::ToMarkable(r.front().value());
            if (key && value) {
                tracer->callback(tracer, memberOf,
                                 JS::GCCellPtr(r.front().key()),
                                 JS::GCCellPtr(r.front().value()));
            }
        }
    }

    
    void entryMoved(Enum& eArg, const Key& k) {
        typedef typename HashMap<typename Unbarriered<Key>::type,
                                 typename Unbarriered<Value>::type,
                                 typename Unbarriered<HashPolicy>::type,
                                 RuntimeAllocPolicy>::Enum UnbarrieredEnum;
        UnbarrieredEnum& e = reinterpret_cast<UnbarrieredEnum&>(eArg);
        e.rekeyFront(reinterpret_cast<const typename Unbarriered<Key>::type&>(k));
    }

protected:
    void assertEntriesNotAboutToBeFinalized() {
#if DEBUG
        for (Range r = Base::all(); !r.empty(); r.popFront()) {
            Key k(r.front().key());
            MOZ_ASSERT(!gc::IsAboutToBeFinalized(&k));
            MOZ_ASSERT(!gc::IsAboutToBeFinalized(&r.front().value()));
            MOZ_ASSERT(k == r.front().key());
        }
#endif
    }
};





template <class Key, class Value>
static inline gc::HashKeyRef<HashMap<Key, Value, DefaultHasher<Key>, RuntimeAllocPolicy>, Key>
UnbarrieredRef(WeakMap<PreBarriered<Key>, RelocatablePtr<Value>>* map, Key key)
{
    






    typedef typename WeakMap<PreBarriered<Key>, RelocatablePtr<Value>>::Base BaseMap;
    auto baseMap = static_cast<BaseMap*>(map);
    typedef HashMap<Key, Value, DefaultHasher<Key>, RuntimeAllocPolicy> UnbarrieredMap;
    typedef gc::HashKeyRef<UnbarrieredMap, Key> UnbarrieredKeyRef;
    return UnbarrieredKeyRef(reinterpret_cast<UnbarrieredMap*>(baseMap), key);
}



extern JSObject*
InitBareWeakMapCtor(JSContext* cx, js::HandleObject obj);

extern bool
WeakMap_has(JSContext* cx, unsigned argc, Value* vp);

extern bool
WeakMap_get(JSContext* cx, unsigned argc, Value* vp);

extern bool
WeakMap_set(JSContext* cx, unsigned argc, Value* vp);

extern bool
WeakMap_delete(JSContext* cx, unsigned argc, Value* vp);

extern bool
WeakMap_clear(JSContext* cx, unsigned argc, Value* vp);

extern JSObject*
InitWeakMapClass(JSContext* cx, HandleObject obj);

} 

#endif 
