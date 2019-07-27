





#ifndef js_TracingAPI_h
#define js_TracingAPI_h

#include "jsalloc.h"
#include "jspubtd.h"

#include "js/HashTable.h"

class JS_PUBLIC_API(JSTracer);

namespace JS {
class JS_PUBLIC_API(CallbackTracer);
template <typename T> class Heap;
template <typename T> class TenuredHeap;
}










enum JSGCTraceKind
{
    
    
    
    JSTRACE_OBJECT = 0x00,
    JSTRACE_STRING = 0x01,
    JSTRACE_SYMBOL = 0x02,
    JSTRACE_SCRIPT = 0x03,

    
    JSTRACE_SHAPE = 0x04,

    
    JSTRACE_NULL = 0x06,

    
    JSTRACE_OUTOFLINE = 0x07,

    
    JSTRACE_BASE_SHAPE = 0x0F,
    JSTRACE_JITCODE = 0x1F,
    JSTRACE_LAZY_SCRIPT = 0x2F,
    JSTRACE_OBJECT_GROUP = 0x3F,

    JSTRACE_LAST = JSTRACE_OBJECT_GROUP
};

namespace JS {

JS_FRIEND_API(const char *)
GCTraceKindToAscii(JSGCTraceKind kind);
}















typedef void
(* JSTraceCallback)(JS::CallbackTracer *trc, void **thingp, JSGCTraceKind kind);



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
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    void setTracingDetails(JSTraceNamePrinter printer, const void *arg, size_t index) {
        debugPrinter_ = printer;
        debugPrintArg_ = arg;
        debugPrintIndex_ = index;
    }

    void setTracingIndex(const char *name, size_t index) {
        setTracingDetails(nullptr, (void *)name, index);
    }

    void setTracingName(const char *name) {
        setTracingDetails(nullptr, (void *)name, InvalidIndex);
    }

    
    void clearTracingDetails() {
        debugPrinter_ = nullptr;
        debugPrintArg_ = nullptr;
    }

    const static size_t InvalidIndex = size_t(-1);

    
    bool hasTracingDetails() const;

    
    
    const char *tracingName(const char *fallback) const;

    
    
    const char *getTracingEdgeName(char *buffer, size_t bufferSize);

    
    JSTraceNamePrinter debugPrinter() const;
    const void *debugPrintArg() const;
    size_t debugPrintIndex() const;

    
    JSRuntime *runtime() const { return runtime_; }

    
    WeakMapTraceKind eagerlyTraceWeakMaps() const { return eagerlyTraceWeakMaps_; }

#ifdef JS_GC_ZEAL
    
    
    
    
    
    
    void setTracingLocation(void *location);
    void unsetTracingLocation();
    void **tracingLocation(void **thingp);
#else
    void setTracingLocation(void *location) {}
    void unsetTracingLocation() {}
    void **tracingLocation(void **thingp) { return nullptr; }
#endif

    
    enum TracerKindTag {
        MarkingTracer,
        CallbackTracer
    };
    bool isMarkingTracer() const { return tag == MarkingTracer; }
    bool isCallbackTracer() const { return tag == CallbackTracer; }
    inline JS::CallbackTracer *asCallbackTracer();

  protected:
    JSTracer(JSRuntime *rt, TracerKindTag tag,
             WeakMapTraceKind weakTraceKind = TraceWeakMapValues);

  private:
    JSRuntime           *runtime_;
    TracerKindTag       tag;
    JSTraceNamePrinter  debugPrinter_;
    const void          *debugPrintArg_;
    size_t              debugPrintIndex_;
    WeakMapTraceKind    eagerlyTraceWeakMaps_;
#ifdef JS_GC_ZEAL
    void                *realLocation_;
#endif
};

namespace JS {

class JS_PUBLIC_API(CallbackTracer) : public JSTracer
{
  public:
    CallbackTracer(JSRuntime *rt, JSTraceCallback traceCallback,
                   WeakMapTraceKind weakTraceKind = TraceWeakMapValues)
      : JSTracer(rt, JSTracer::CallbackTracer, weakTraceKind), callback(traceCallback)
    {}

    
    void setTraceCallback(JSTraceCallback traceCallback);

    
    bool hasCallback(JSTraceCallback maybeCallback) const {
        return maybeCallback == callback;
    }

    
    void invoke(void **thing, JSGCTraceKind kind) {
        callback(this, thing, kind);
    }

  private:
    
    
    JSTraceCallback callback;
};

} 

JS::CallbackTracer *
JSTracer::asCallbackTracer()
{
    MOZ_ASSERT(isCallbackTracer());
    return static_cast<JS::CallbackTracer *>(this);
}













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
        e.rekeyFront(updated);
}



extern JS_PUBLIC_API(void)
JS_CallTenuredObjectTracer(JSTracer *trc, JS::TenuredHeap<JSObject *> *objp, const char *name);

extern JS_PUBLIC_API(void)
JS_TraceChildren(JSTracer *trc, void *thing, JSGCTraceKind kind);

extern JS_PUBLIC_API(void)
JS_TraceRuntime(JSTracer *trc);

namespace JS {
typedef js::HashSet<Zone *, js::DefaultHasher<Zone *>, js::SystemAllocPolicy> ZoneSet;
}



extern JS_PUBLIC_API(void)
JS_TraceIncomingCCWs(JSTracer *trc, const JS::ZoneSet &zones);

extern JS_PUBLIC_API(void)
JS_GetTraceThingInfo(char *buf, size_t bufsize, JSTracer *trc,
                     void *thing, JSGCTraceKind kind, bool includeDetails);

#endif 
