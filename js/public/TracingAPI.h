





#ifndef js_TracingAPI_h
#define js_TracingAPI_h

#include "mozilla/NullPtr.h"

#include "jspubtd.h"

class JS_PUBLIC_API(JSTracer);

namespace JS {
template <typename T> class Heap;
template <typename T> class TenuredHeap;
}















typedef void
(* JSTraceCallback)(JSTracer *trc, void **thingp, JSGCTraceKind kind);



typedef void
(* JSTraceNamePrinter)(JSTracer *trc, char *buf, size_t bufsize);

enum WeakMapTraceKind {
    DoNotTraceWeakMaps = 0,
    TraceWeakMapValues = 1,
    TraceWeakMapKeysValues = 2
};

class JS_PUBLIC_API(JSTracer)
{
  public:
    JSTracer(JSRuntime *rt, JSTraceCallback traceCallback,
             WeakMapTraceKind weakTraceKind = TraceWeakMapValues);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    void setTracingDetails(JSTraceNamePrinter printer, const void *arg, size_t index) {
        debugPrinter_ = printer;
        debugPrintArg_ = arg;
        debugPrintIndex_ = index;
    }

    void setTracingIndex(const char *name, size_t index) {
        setTracingDetails(nullptr, (void *)name, index);
    }

    void setTracingName(const char *name) {
        setTracingDetails(nullptr, (void *)name, size_t(-1));
    }

    
    void clearTracingDetails() {
        debugPrinter_ = nullptr;
        debugPrintArg_ = nullptr;
    }

    
    bool hasTracingDetails() const;

    
    
    const char *tracingName(const char *fallback) const;

    
    
    const char *getTracingEdgeName(char *buffer, size_t bufferSize);

    
    JSTraceNamePrinter debugPrinter() const;
    const void *debugPrintArg() const;
    size_t debugPrintIndex() const;

    
    JSRuntime *runtime() const { return runtime_; }

    
    WeakMapTraceKind eagerlyTraceWeakMaps() const { return eagerlyTraceWeakMaps_; }

    
    void setTraceCallback(JSTraceCallback traceCallback);

#ifdef JS_GC_ZEAL
    
    
    
    
    
    
    void setTracingLocation(void *location);
    void unsetTracingLocation();
    void **tracingLocation(void **thingp);
#else
    void setTracingLocation(void *location) {}
    void unsetTracingLocation() {}
    void **tracingLocation(void **thingp) { return nullptr; }
#endif

    
    
    JSTraceCallback     callback;

  private:
    JSRuntime           *runtime_;
    JSTraceNamePrinter  debugPrinter_;
    const void          *debugPrintArg_;
    size_t              debugPrintIndex_;
    WeakMapTraceKind    eagerlyTraceWeakMaps_;
#ifdef JS_GC_ZEAL
    void                *realLocation_;
#endif
};













extern JS_PUBLIC_API(void)
JS_CallValueTracer(JSTracer *trc, JS::Heap<JS::Value> *valuep, const char *name);

extern JS_PUBLIC_API(void)
JS_CallIdTracer(JSTracer *trc, JS::Heap<jsid> *idp, const char *name);

extern JS_PUBLIC_API(void)
JS_CallObjectTracer(JSTracer *trc, JS::Heap<JSObject *> *objp, const char *name);

extern JS_PUBLIC_API(void)
JS_CallStringTracer(JSTracer *trc, JS::Heap<JSString *> *strp, const char *name);

extern JS_PUBLIC_API(void)
JS_CallScriptTracer(JSTracer *trc, JS::Heap<JSScript *> *scriptp, const char *name);

extern JS_PUBLIC_API(void)
JS_CallFunctionTracer(JSTracer *trc, JS::Heap<JSFunction *> *funp, const char *name);




extern JS_PUBLIC_API(void)
JS_CallUnbarrieredValueTracer(JSTracer *trc, JS::Value *valuep, const char *name);

extern JS_PUBLIC_API(void)
JS_CallUnbarrieredIdTracer(JSTracer *trc, jsid *idp, const char *name);

extern JS_PUBLIC_API(void)
JS_CallUnbarrieredObjectTracer(JSTracer *trc, JSObject **objp, const char *name);

extern JS_PUBLIC_API(void)
JS_CallUnbarrieredStringTracer(JSTracer *trc, JSString **strp, const char *name);

extern JS_PUBLIC_API(void)
JS_CallUnbarrieredScriptTracer(JSTracer *trc, JSScript **scriptp, const char *name);

template <typename HashSetEnum>
inline void
JS_CallHashSetObjectTracer(JSTracer *trc, HashSetEnum &e, JSObject *const &key, const char *name)
{
    JSObject *updated = key;
    trc->setTracingLocation(reinterpret_cast<void *>(&const_cast<JSObject *&>(key)));
    JS_CallUnbarrieredObjectTracer(trc, &updated, name);
    if (updated != key)
        e.rekeyFront(key, updated);
}



extern JS_PUBLIC_API(void)
JS_CallTenuredObjectTracer(JSTracer *trc, JS::TenuredHeap<JSObject *> *objp, const char *name);

extern JS_PUBLIC_API(void)
JS_TraceChildren(JSTracer *trc, void *thing, JSGCTraceKind kind);

extern JS_PUBLIC_API(void)
JS_TraceRuntime(JSTracer *trc);

extern JS_PUBLIC_API(void)
JS_GetTraceThingInfo(char *buf, size_t bufsize, JSTracer *trc,
                     void *thing, JSGCTraceKind kind, bool includeDetails);

#endif 
