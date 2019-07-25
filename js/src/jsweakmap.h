






#ifndef jsweakmap_h___
#define jsweakmap_h___

#include "jsapi.h"
#include "jsfriendapi.h"
#include "jscntxt.h"
#include "jsobj.h"

#include "gc/Marking.h"
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

  public:
    typedef typename Base::Enum Enum;
    typedef typename Base::Range Range;

    explicit WeakMap(JSRuntime *rt, JSObject *memOf=NULL) : Base(rt), WeakMapBase(memOf) { }
    explicit WeakMap(JSContext *cx, JSObject *memOf=NULL) : Base(cx), WeakMapBase(memOf) { }

    
    Range nondeterministicAll() {
        return Base::all();
    }

  private:
    bool markValue(JSTracer *trc, Value *x) {
        if (gc::IsMarked(x))
            return false;
        gc::Mark(trc, x, "WeakMap entry");
        return true;
    }

    void nonMarkingTrace(JSTracer *trc) {
        for (Range r = Base::all(); !r.empty(); r.popFront())
            markValue(trc, &r.front().value);
    }

    bool markIteratively(JSTracer *trc) {
        bool markedAny = false;
        for (Enum e(*this); !e.empty(); e.popFront()) {
            
            Key k(e.front().key);
            bool keyIsMarked = gc::IsMarked(&k);
            if (keyIsMarked) {
                if (markValue(trc, &e.front().value))
                    markedAny = true;
                e.rekeyFront(k);
            }
        }
        return markedAny;
    }

    void sweep(JSTracer *trc) {
        
        for (Enum e(*this); !e.empty(); e.popFront()) {
            Key k(e.front().key);
            if (!gc::IsMarked(&k))
                e.removeFront();
        }

#if DEBUG
        



        for (Range r = Base::all(); !r.empty(); r.popFront()) {
            Key k(r.front().key);
            Value v(r.front().value);
            JS_ASSERT(gc::IsMarked(&k));
            JS_ASSERT(gc::IsMarked(&v));
            JS_ASSERT(k == r.front().key);
            JS_ASSERT(v == r.front().value);
        }
#endif
    }

    
    void traceMappings(WeakMapTracer *tracer) {
        for (Range r = Base::all(); !r.empty(); r.popFront()) {
            gc::Cell *key = gc::ToMarkable(r.front().key);
            gc::Cell *value = gc::ToMarkable(r.front().value);
            if (key && value) {
                tracer->callback(tracer, memberOf,
                                 key, gc::TraceKind(r.front().key),
                                 value, gc::TraceKind(r.front().value));
            }
        }
    }
};

} 

extern JSObject *
js_InitWeakMapClass(JSContext *cx, JSObject *obj);

#endif
