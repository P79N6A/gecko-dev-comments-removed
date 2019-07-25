








































#ifndef jsweakmap_h___
#define jsweakmap_h___

#include "jsapi.h"
#include "jsfriendapi.h"
#include "jscntxt.h"
#include "jsobj.h"
#include "jsgcmark.h"

#include "js/HashTable.h"

namespace js {
















































static WeakMapBase * const WeakMapNotInList = reinterpret_cast<WeakMapBase *>(1);

typedef Vector<WeakMapBase *, 0, SystemAllocPolicy> WeakMapVector;



class WeakMapBase {
  public:
    WeakMapBase(JSObject *memOf) : memberOf(memOf), next(WeakMapNotInList) { }
    virtual ~WeakMapBase() { }

    void trace(JSTracer *tracer) {
        if (IS_GC_MARKING_TRACER(tracer)) {
            
            
            
            
            JS_ASSERT(!tracer->eagerlyTraceWeakMaps);

            
            
            if (next == WeakMapNotInList) {
                JSRuntime *rt = tracer->runtime;
                next = rt->gcWeakMapList;
                rt->gcWeakMapList = this;
            }
        } else {
            
            
            
            
            if (tracer->eagerlyTraceWeakMaps)
                nonMarkingTrace(tracer);
        }
    }

    

    
    
    
    
    static bool markAllIteratively(JSTracer *tracer);

    
    
    static void sweepAll(JSTracer *tracer);

    
    static void traceAllMappings(WeakMapTracer *tracer);

    void check() { JS_ASSERT(next == WeakMapNotInList); }

    
    static void resetWeakMapList(JSRuntime *rt);

    
    static bool saveWeakMapList(JSRuntime *rt, WeakMapVector &vector);
    static void restoreWeakMapList(JSRuntime *rt, WeakMapVector &vector);

  protected:
    
    
    virtual void nonMarkingTrace(JSTracer *tracer) = 0;
    virtual bool markIteratively(JSTracer *tracer) = 0;
    virtual void sweep(JSTracer *tracer) = 0;
    virtual void traceMappings(WeakMapTracer *tracer) = 0;

    
    JSObject *memberOf;

  private:
    
    
    
    
    
    WeakMapBase *next;
};

template <class Key, class Value,
          class HashPolicy = DefaultHasher<Key> >
class WeakMap : public HashMap<Key, Value, HashPolicy, RuntimeAllocPolicy>, public WeakMapBase {
  private:
    typedef HashMap<Key, Value, HashPolicy, RuntimeAllocPolicy> Base;
    typedef typename Base::Enum Enum;

  public:
    typedef typename Base::Range Range;

    explicit WeakMap(JSRuntime *rt, JSObject *memOf=NULL) : Base(rt), WeakMapBase(memOf) { }
    explicit WeakMap(JSContext *cx, JSObject *memOf=NULL) : Base(cx), WeakMapBase(memOf) { }

    
    Range nondeterministicAll() {
        return Base::all();
    }

  private:
    bool IsMarked(const HeapValue &x) {
        if (x.isMarkable())
            return !IsAboutToBeFinalized(x);
        return true;
    }
    bool IsMarked(const HeapPtrObject &x) {
        return !IsAboutToBeFinalized(x);
    }
    bool IsMarked(const HeapPtrScript&x) {
        return !IsAboutToBeFinalized(x);
    }

    bool Mark(JSTracer *trc, HeapValue *x) {
        if (IsMarked(*x))
            return false;
        js::gc::MarkValue(trc, x, "WeakMap entry value");
        return true;
    }
    bool Mark(JSTracer *trc, HeapPtrObject *x) {
        if (IsMarked(*x))
            return false;
        js::gc::MarkObject(trc, x, "WeakMap entry value");
        return true;
    }
    bool Mark(JSTracer *trc, HeapPtrScript *x) {
        if (IsMarked(*x))
            return false;
        js::gc::MarkScript(trc, x, "WeakMap entry value");
        return true;
    }

    void nonMarkingTrace(JSTracer *trc) {
        for (Range r = Base::all(); !r.empty(); r.popFront())
            Mark(trc, &r.front().value);
    }

    bool markIteratively(JSTracer *trc) {
        bool markedAny = false;
        for (Range r = Base::all(); !r.empty(); r.popFront()) {
            
            if (IsMarked(r.front().key) && Mark(trc, &r.front().value))
                markedAny = true;
            JS_ASSERT_IF(IsMarked(r.front().key), IsMarked(r.front().value));
        }
        return markedAny;
    }

    void sweep(JSTracer *trc) {
        
        for (Enum e(*this); !e.empty(); e.popFront()) {
            if (!IsMarked(e.front().key))
                e.removeFront();
        }

#if DEBUG
        



        for (Range r = Base::all(); !r.empty(); r.popFront()) {
            JS_ASSERT(IsMarked(r.front().key));
            JS_ASSERT(IsMarked(r.front().value));
        }
#endif
    }

    void CallTracer(WeakMapTracer *trc, const HeapPtrObject &k, const HeapValue &v) {
        if (v.isMarkable())
            trc->callback(trc, memberOf, k.get(), JSTRACE_OBJECT, v.toGCThing(), v.gcKind());
    }
    void CallTracer(WeakMapTracer *trc, const HeapPtrObject &k, const HeapPtrObject &v) {
        trc->callback(trc, memberOf, k.get(), JSTRACE_OBJECT, v.get(), JSTRACE_OBJECT);
    }
    void CallTracer(WeakMapTracer *trc, const HeapPtrScript &k, const HeapPtrObject &v) {
        trc->callback(trc, memberOf, k.get(), JSTRACE_SCRIPT, v.get(), JSTRACE_OBJECT);
    }

    
    void traceMappings(WeakMapTracer *tracer) {
        for (Range r = Base::all(); !r.empty(); r.popFront())
            CallTracer(tracer, r.front().key, r.front().value);
    }
};

} 

extern JSObject *
js_InitWeakMapClass(JSContext *cx, JSObject *obj);

#endif
