





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

    
    JSTRACE_OBJECT_GROUP = 0x05,

    
    JSTRACE_NULL = 0x06,

    
    JSTRACE_OUTOFLINE = 0x07,

    
    JSTRACE_BASE_SHAPE = 0x0F,
    JSTRACE_JITCODE = 0x1F,
    JSTRACE_LAZY_SCRIPT = 0x2F,

    JSTRACE_LAST = JSTRACE_OBJECT_GROUP
};

namespace JS {

JS_FRIEND_API(const char*)
GCTraceKindToAscii(JSGCTraceKind kind);
}















typedef void
(* JSTraceCallback)(JS::CallbackTracer* trc, void** thingp, JSGCTraceKind kind);

enum WeakMapTraceKind {
    DoNotTraceWeakMaps = 0,
    TraceWeakMapValues = 1,
    TraceWeakMapKeysValues = 2
};

class JS_PUBLIC_API(JSTracer)
{
  public:
    
    JSRuntime* runtime() const { return runtime_; }

    
    WeakMapTraceKind eagerlyTraceWeakMaps() const { return eagerlyTraceWeakMaps_; }

    
    enum TracerKindTag {
        MarkingTracer,
        CallbackTracer
    };
    bool isMarkingTracer() const { return tag_ == MarkingTracer; }
    bool isCallbackTracer() const { return tag_ == CallbackTracer; }
    inline JS::CallbackTracer* asCallbackTracer();

  protected:
    JSTracer(JSRuntime* rt, TracerKindTag tag,
             WeakMapTraceKind weakTraceKind = TraceWeakMapValues)
      : runtime_(rt), tag_(tag), eagerlyTraceWeakMaps_(weakTraceKind)
    {}

  private:
    JSRuntime*          runtime_;
    TracerKindTag       tag_;
    WeakMapTraceKind    eagerlyTraceWeakMaps_;
};

namespace JS {

class AutoTracingName;
class AutoTracingIndex;
class AutoTracingCallback;
class AutoOriginalTraceLocation;

class JS_PUBLIC_API(CallbackTracer) : public JSTracer
{
  public:
    CallbackTracer(JSRuntime* rt, JSTraceCallback traceCallback,
                   WeakMapTraceKind weakTraceKind = TraceWeakMapValues)
      : JSTracer(rt, JSTracer::CallbackTracer, weakTraceKind), callback(traceCallback),
        contextName_(nullptr), contextIndex_(InvalidIndex), contextFunctor_(nullptr),
        contextRealLocation_(nullptr)
    {}

    
    void setTraceCallback(JSTraceCallback traceCallback) {
        callback = traceCallback;
    }

    
    bool hasCallback(JSTraceCallback maybeCallback) const {
        return maybeCallback == callback;
    }

    
    void invoke(void** thing, JSGCTraceKind kind) {
        callback(this, thing, kind);
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    const char* contextName() const { MOZ_ASSERT(contextName_); return contextName_; }

    
    
    
    const static size_t InvalidIndex = size_t(-1);
    size_t contextIndex() const { return contextIndex_; }

    
    
    
    
    
    void getTracingEdgeName(char* buffer, size_t bufferSize);

    
    
    
    
    
    class ContextFunctor {
      public:
        virtual void operator()(CallbackTracer* trc, char* buf, size_t bufsize) = 0;
    };

    
    
    
    
    void*const* tracingLocation(void** thingp) {
        return contextRealLocation_ ? contextRealLocation_ : thingp;
    }

  private:
    
    
    JSTraceCallback callback;

    friend class AutoTracingName;
    const char* contextName_;

    friend class AutoTracingIndex;
    size_t contextIndex_;

    friend class AutoTracingDetails;
    ContextFunctor* contextFunctor_;

    friend class AutoOriginalTraceLocation;
    void*const* contextRealLocation_;
};


class AutoTracingName
{
    CallbackTracer* trc_;
    const char* prior_;

  public:
    AutoTracingName(CallbackTracer* trc, const char* name) : trc_(trc), prior_(trc->contextName_) {
        MOZ_ASSERT(name);
        trc->contextName_ = name;
    }
    ~AutoTracingName() {
        MOZ_ASSERT(trc_->contextName_);
        trc_->contextName_ = prior_;
    }
};


class AutoTracingIndex
{
    CallbackTracer* trc_;

  public:
    explicit AutoTracingIndex(JSTracer* trc, size_t initial = 0) : trc_(nullptr) {
        if (trc->isCallbackTracer()) {
            trc_ = trc->asCallbackTracer();
            MOZ_ASSERT(trc_->contextIndex_ == CallbackTracer::InvalidIndex);
            trc_->contextIndex_ = initial;
        }
    }
    ~AutoTracingIndex() {
        if (trc_) {
            MOZ_ASSERT(trc_->contextIndex_ != CallbackTracer::InvalidIndex);
            trc_->contextIndex_ = CallbackTracer::InvalidIndex;
        }
    }

    void operator++() {
        if (trc_) {
            MOZ_ASSERT(trc_->contextIndex_ != CallbackTracer::InvalidIndex);
            ++trc_->contextIndex_;
        }
    }
};



class AutoTracingDetails
{
    CallbackTracer* trc_;

  public:
    AutoTracingDetails(JSTracer* trc, CallbackTracer::ContextFunctor& func) : trc_(nullptr) {
        if (trc->isCallbackTracer()) {
            trc_ = trc->asCallbackTracer();
            MOZ_ASSERT(trc_->contextFunctor_ == nullptr);
            trc_->contextFunctor_ = &func;
        }
    }
    ~AutoTracingDetails() {
        if (trc_) {
            MOZ_ASSERT(trc_->contextFunctor_);
            trc_->contextFunctor_ = nullptr;
        }
    }
};






class AutoOriginalTraceLocation
{
#ifdef JS_GC_ZEAL
    CallbackTracer* trc_;

  public:
    template <typename T>
    AutoOriginalTraceLocation(JSTracer* trc, T*const* realLocation) : trc_(nullptr) {
        if (trc->isCallbackTracer() && trc->asCallbackTracer()->contextRealLocation_ == nullptr) {
            trc_ = trc->asCallbackTracer();
            trc_->contextRealLocation_ = reinterpret_cast<void*const*>(realLocation);
        }
    }
    ~AutoOriginalTraceLocation() {
        if (trc_) {
            MOZ_ASSERT(trc_->contextRealLocation_);
            trc_->contextRealLocation_ = nullptr;
        }
    }
#else
  public:
    template <typename T>
    AutoOriginalTraceLocation(JSTracer* trc, T*const* realLocation) {}
#endif
};

} 

JS::CallbackTracer*
JSTracer::asCallbackTracer()
{
    MOZ_ASSERT(isCallbackTracer());
    return static_cast<JS::CallbackTracer*>(this);
}













extern JS_PUBLIC_API(void)
JS_CallValueTracer(JSTracer* trc, JS::Heap<JS::Value>* valuep, const char* name);

extern JS_PUBLIC_API(void)
JS_CallIdTracer(JSTracer* trc, JS::Heap<jsid>* idp, const char* name);

extern JS_PUBLIC_API(void)
JS_CallObjectTracer(JSTracer* trc, JS::Heap<JSObject*>* objp, const char* name);

extern JS_PUBLIC_API(void)
JS_CallStringTracer(JSTracer* trc, JS::Heap<JSString*>* strp, const char* name);

extern JS_PUBLIC_API(void)
JS_CallScriptTracer(JSTracer* trc, JS::Heap<JSScript*>* scriptp, const char* name);

extern JS_PUBLIC_API(void)
JS_CallFunctionTracer(JSTracer* trc, JS::Heap<JSFunction*>* funp, const char* name);




extern JS_PUBLIC_API(void)
JS_CallUnbarrieredValueTracer(JSTracer* trc, JS::Value* valuep, const char* name);

extern JS_PUBLIC_API(void)
JS_CallUnbarrieredIdTracer(JSTracer* trc, jsid* idp, const char* name);

extern JS_PUBLIC_API(void)
JS_CallUnbarrieredObjectTracer(JSTracer* trc, JSObject** objp, const char* name);

extern JS_PUBLIC_API(void)
JS_CallUnbarrieredStringTracer(JSTracer* trc, JSString** strp, const char* name);

extern JS_PUBLIC_API(void)
JS_CallUnbarrieredScriptTracer(JSTracer* trc, JSScript** scriptp, const char* name);

template <typename HashSetEnum>
inline void
JS_CallHashSetObjectTracer(JSTracer* trc, HashSetEnum& e, JSObject* const& key, const char* name)
{
    JSObject* updated = key;
    JS::AutoOriginalTraceLocation reloc(trc, &key);
    JS_CallUnbarrieredObjectTracer(trc, &updated, name);
    if (updated != key)
        e.rekeyFront(updated);
}



extern JS_PUBLIC_API(void)
JS_CallTenuredObjectTracer(JSTracer* trc, JS::TenuredHeap<JSObject*>* objp, const char* name);

extern JS_PUBLIC_API(void)
JS_TraceChildren(JSTracer* trc, void* thing, JSGCTraceKind kind);

extern JS_PUBLIC_API(void)
JS_TraceRuntime(JSTracer* trc);

namespace JS {
typedef js::HashSet<Zone*, js::DefaultHasher<Zone*>, js::SystemAllocPolicy> ZoneSet;
}



extern JS_PUBLIC_API(void)
JS_TraceIncomingCCWs(JSTracer* trc, const JS::ZoneSet& zones);

extern JS_PUBLIC_API(void)
JS_GetTraceThingInfo(char* buf, size_t bufsize, JSTracer* trc,
                     void* thing, JSGCTraceKind kind, bool includeDetails);

#endif
