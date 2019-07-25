








































#ifndef jsweakmap_h___
#define jsweakmap_h___

#include "jsapi.h"
#include "jscntxt.h"
#include "jsobj.h"

namespace js {


















































template <class Key, class Value> class DefaultMarkPolicy;



class WeakMapBase {
  public:
    WeakMapBase() : next(NULL) { }

    void trace(JSTracer *tracer) {
        if (IS_GC_MARKING_TRACER(tracer)) {
            
            
            
            
            JSRuntime *rt = tracer->context->runtime;
            next = rt->gcWeakMapList;
            rt->gcWeakMapList = this;
        } else {
            
            
            
            
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
    typedef typename Base::Range Range;
    typedef typename Base::Enum Enum;

  public:
    WeakMap(JSContext *cx) : Base(cx) { }

  private:
    void nonMarkingTrace(JSTracer *tracer) {
        MarkPolicy t(tracer);
        for (Range r = Base::all(); !r.empty(); r.popFront()) {
            t.markKey(r.front().key, "WeakMap entry key");
            t.markValue(r.front().value, "WeakMap entry value");
        }
    }

    bool markIteratively(JSTracer *tracer) {
        MarkPolicy t(tracer);
        bool markedAny = false;
        for (Range r = Base::all(); !r.empty(); r.popFront()) {
            
            if (!t.valueMarked(r.front().value) && t.keyMarked(r.front().key)) {
                t.markValue(r.front().value, "WeakMap entry with live key");
                
                markedAny = true;
            }
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
    void markKey(JSObject *k, const char *description) {
        js::gc::MarkObject(tracer, *k, description);
    }
    void markValue(const Value &v, const char *description) {
        js::gc::MarkValue(tracer, v, description);
    }
};


extern Class WeakMapClass;

}

extern JSObject *
js_InitWeakMapClass(JSContext *cx, JSObject *obj);

#endif
