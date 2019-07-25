








































#ifndef jsweakmap_h___
#define jsweakmap_h___

#include "jsapi.h"
#include "jscntxt.h"
#include "jsobj.h"
#include "jsgcmark.h"

#include "js/HashTable.h"

namespace js {


















































































template <class Key, class Value> class DefaultMarkPolicy;



class WeakMapBase {
  public:
    WeakMapBase() : next(NULL) { }
    virtual ~WeakMapBase() { }

    void trace(JSTracer *tracer) {
        if (IS_GC_MARKING_TRACER(tracer)) {
            
            
            
            
            JS_ASSERT(!tracer->eagerlyTraceWeakMaps);
            JSRuntime *rt = tracer->context->runtime;
            next = rt->gcWeakMapList;
            rt->gcWeakMapList = this;
        } else {
            
            
            
            
            if (tracer->eagerlyTraceWeakMaps)
                nonMarkingTrace(tracer);
        }
    }

    

    
    
    
    
    static bool markAllIteratively(JSTracer *tracer);

    
    
    static void sweepAll(JSTracer *tracer);

  protected:
    
    
    virtual void nonMarkingTrace(JSTracer *tracer) = 0;
    virtual bool markIteratively(JSTracer *tracer) = 0;
    virtual void sweep(JSTracer *tracer) = 0;

  private:
    
    
    WeakMapBase *next;
};

template <class Key, class Value,
          class HashPolicy = DefaultHasher<Key>,
          class MarkPolicy = DefaultMarkPolicy<Key, Value> >
class WeakMap : public HashMap<Key, Value, HashPolicy, RuntimeAllocPolicy>, public WeakMapBase {
  private:
    typedef HashMap<Key, Value, HashPolicy, RuntimeAllocPolicy> Base;
    typedef typename Base::Enum Enum;

  public:
    typedef typename Base::Range Range;

    explicit WeakMap(JSRuntime *rt) : Base(rt) { }
    explicit WeakMap(JSContext *cx) : Base(cx) { }

    
    Range nondeterministicAll() {
        return Base::all();
    }

  private:
    void nonMarkingTrace(JSTracer *tracer) {
        MarkPolicy t(tracer);
        for (Range r = Base::all(); !r.empty(); r.popFront())
            t.markEntry(r.front().value);
    }

    bool markIteratively(JSTracer *tracer) {
        MarkPolicy t(tracer);
        bool markedAny = false;
        for (Range r = Base::all(); !r.empty(); r.popFront()) {
            
            if (t.markEntryIfLive(r.front().key, r.front().value)) {
                
                markedAny = true;
            }
            JS_ASSERT_IF(t.keyMarked(r.front().key), t.valueMarked(r.front().value));
        }
        return markedAny;
    }

    void sweep(JSTracer *tracer) {
        MarkPolicy t(tracer);

        
        for (Enum e(*this); !e.empty(); e.popFront()) {
            if (!t.keyMarked(e.front().key))
                e.removeFront();
        }

#if DEBUG
        



        for (Range r = Base::all(); !r.empty(); r.popFront()) {
            JS_ASSERT(t.keyMarked(r.front().key));
            JS_ASSERT(t.valueMarked(r.front().value));
        }
#endif
    }
};







template <>
class DefaultMarkPolicy<JSObject *, Value> {
  private:
    JSTracer *tracer;
  public:
    DefaultMarkPolicy(JSTracer *t) : tracer(t) { }
    bool keyMarked(JSObject *k) { return !IsAboutToBeFinalized(tracer->context, k); }
    bool valueMarked(const Value &v) {
        if (v.isMarkable())
            return !IsAboutToBeFinalized(tracer->context, v.toGCThing());
        return true;
    }
  private:
    bool markUnmarkedValue(const Value &v) {
        if (valueMarked(v))
            return false;
        js::gc::MarkValue(tracer, v, "WeakMap entry value");
        return true;
    }

    
    
    bool overrideKeyMarking(JSObject *k) {
        
        
        if (!IS_GC_MARKING_TRACER(tracer))
            return false;
        return k->getClass()->ext.isWrappedNative;
    }
  public:
    bool markEntryIfLive(JSObject *k, const Value &v) {
        if (keyMarked(k))
            return markUnmarkedValue(v);
        if (!overrideKeyMarking(k))
            return false;
        js::gc::MarkObject(tracer, *k, "WeakMap entry wrapper key");
        markUnmarkedValue(v);
        return true;
    }
    void markEntry(const Value &v) {
        js::gc::MarkValue(tracer, v, "WeakMap entry value");
    }
};

template <>
class DefaultMarkPolicy<gc::Cell *, JSObject *> {
  protected:
    JSTracer *tracer;
  public:
    DefaultMarkPolicy(JSTracer *t) : tracer(t) { }
    bool keyMarked(gc::Cell *k)   { return !IsAboutToBeFinalized(tracer->context, k); }
    bool valueMarked(JSObject *v) { return !IsAboutToBeFinalized(tracer->context, v); }
    bool markEntryIfLive(gc::Cell *k, JSObject *v) {
        if (keyMarked(k) && !valueMarked(v)) {
            js::gc::MarkObject(tracer, *v, "WeakMap entry value");
            return true;
        }
        return false;
    }
    void markEntry(JSObject *v) {
        js::gc::MarkObject(tracer, *v, "WeakMap entry value");
    }
};








typedef DefaultMarkPolicy<gc::Cell *, JSObject *> CrossCompartmentMarkPolicy;

}

extern JSObject *
js_InitWeakMapClass(JSContext *cx, JSObject *obj);

#endif
