








































#ifndef jsweakmap_h___
#define jsweakmap_h___

#include "jsapi.h"
#include "jsfriendapi.h"
#include "jscntxt.h"
#include "jsobj.h"
#include "jsgcmark.h"

#include "js/HashTable.h"

namespace js {

















































template <class Type> class DefaultMarkPolicy;



template <class Key, class Value> class DefaultTracePolicy;



class WeakMapBase {
  public:
    WeakMapBase(JSObject *memOf) : memberOf(memOf), next(NULL) { }
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

    
    static void traceAllMappings(WeakMapTracer *tracer);

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
          class HashPolicy = DefaultHasher<Key>,
          class KeyMarkPolicy = DefaultMarkPolicy<Key>,
          class ValueMarkPolicy = DefaultMarkPolicy<Value>,
          class TracePolicy = DefaultTracePolicy<Key, Value> >
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
    void nonMarkingTrace(JSTracer *trc) {
        ValueMarkPolicy vp(trc);
        for (Range r = Base::all(); !r.empty(); r.popFront())
            vp.mark(r.front().value);
    }

    bool markIteratively(JSTracer *trc) {
        KeyMarkPolicy kp(trc);
        ValueMarkPolicy vp(trc);
        bool markedAny = false;
        for (Range r = Base::all(); !r.empty(); r.popFront()) {
            const Key &k = r.front().key;
            const Value &v = r.front().value;
            
            if (kp.isMarked(k)) {
                markedAny |= vp.mark(v);
            } else if (kp.overrideKeyMarking(k)) {
                
                
                
                kp.mark(k);
                vp.mark(v);
                markedAny = true;
            }
            JS_ASSERT_IF(kp.isMarked(k), vp.isMarked(v));
        }
        return markedAny;
    }

    void sweep(JSTracer *trc) {
        KeyMarkPolicy kp(trc);

        
        for (Enum e(*this); !e.empty(); e.popFront()) {
            if (!kp.isMarked(e.front().key))
                e.removeFront();
        }

#if DEBUG
        ValueMarkPolicy vp(trc);
        



        for (Range r = Base::all(); !r.empty(); r.popFront()) {
            JS_ASSERT(kp.isMarked(r.front().key));
            JS_ASSERT(vp.isMarked(r.front().value));
        }
#endif
    }

    
    void traceMappings(WeakMapTracer *tracer) {
        TracePolicy t(tracer);
        for (Range r = Base::all(); !r.empty(); r.popFront())
            t.traceMapping(memberOf, r.front().key, r.front().value);
    }
};

template <>
class DefaultMarkPolicy<HeapValue> {
  private:
    JSTracer *tracer;
  public:
    DefaultMarkPolicy(JSTracer *t) : tracer(t) { }
    bool isMarked(const HeapValue &x) {
        if (x.isMarkable())
            return !IsAboutToBeFinalized(tracer->context, x);
        return true;
    }
    bool mark(const HeapValue &x) {
        if (isMarked(x))
            return false;
        js::gc::MarkValue(tracer, x, "WeakMap entry");
        return true;
    }
    bool overrideKeyMarking(const HeapValue &k) { return false; }
};

template <>
class DefaultMarkPolicy<HeapPtrObject> {
  private:
    JSTracer *tracer;
  public:
    DefaultMarkPolicy(JSTracer *t) : tracer(t) { }
    bool isMarked(const HeapPtrObject &x) {
        return !IsAboutToBeFinalized(tracer->context, x);
    }
    bool mark(const HeapPtrObject &x) {
        if (isMarked(x))
            return false;
        js::gc::MarkObject(tracer, x, "WeakMap entry");
        return true;
    }
    bool overrideKeyMarking(const HeapPtrObject &k) {
        
        
        if (!IS_GC_MARKING_TRACER(tracer))
            return false;
        return k->getClass()->ext.isWrappedNative;
    }
};

template <>
class DefaultMarkPolicy<HeapPtrScript> {
  private:
    JSTracer *tracer;
  public:
    DefaultMarkPolicy(JSTracer *t) : tracer(t) { }
    bool isMarked(const HeapPtrScript &x) {
        return !IsAboutToBeFinalized(tracer->context, x);
    }
    bool mark(const HeapPtrScript &x) {
        if (isMarked(x))
            return false;
        js::gc::MarkScript(tracer, x, "WeakMap entry");
        return true;
    }
    bool overrideKeyMarking(const HeapPtrScript &k) { return false; }
};



template <>
class DefaultTracePolicy<HeapPtrObject, HeapValue> {
  private:
    WeakMapTracer *tracer;
  public:
    DefaultTracePolicy(WeakMapTracer *t) : tracer(t) { }
    void traceMapping(JSObject *m, const HeapPtr<JSObject> &k, HeapValue &v) {
        if (v.isMarkable())
            tracer->callback(tracer, m, k.get(), JSTRACE_OBJECT, v.toGCThing(), v.gcKind());
    }
};

template <>
class DefaultTracePolicy<HeapPtrObject, HeapPtrObject> {
  private:
    WeakMapTracer *tracer;
  public:
    DefaultTracePolicy(WeakMapTracer *t) : tracer(t) { }
    void traceMapping(JSObject *m, const HeapPtrObject &k, const HeapPtrObject &v) {
        tracer->callback(tracer, m, k.get(), JSTRACE_OBJECT, v.get(), JSTRACE_OBJECT);
    }
};

template <>
class DefaultTracePolicy<HeapPtrScript, HeapPtrObject> {
  private:
    WeakMapTracer *tracer;
  public:
    DefaultTracePolicy(WeakMapTracer *t) : tracer(t) { }
    void traceMapping(JSObject *m, const HeapPtrScript &k, const HeapPtrObject &v) {
        tracer->callback(tracer, m, k.get(), JSTRACE_SCRIPT, v.get(), JSTRACE_OBJECT);
    }
};

}

extern JSObject *
js_InitWeakMapClass(JSContext *cx, JSObject *obj);

#endif
