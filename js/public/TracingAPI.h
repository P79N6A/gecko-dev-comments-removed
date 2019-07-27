





#ifndef js_TracingAPI_h
#define js_TracingAPI_h

#include "jsalloc.h"

#include "js/HashTable.h"
#include "js/HeapAPI.h"
#include "js/TraceKind.h"

class JS_PUBLIC_API(JSTracer);

namespace JS {
class JS_PUBLIC_API(CallbackTracer);
template <typename T> class Heap;
template <typename T> class TenuredHeap;


JS_FRIEND_API(const char*)
GCTraceKindToAscii(JS::TraceKind kind);

} 

namespace js {
class BaseShape;
class LazyScript;
class ObjectGroup;
namespace jit {
class JitCode;
} 
} 

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

    
    enum class TracerKindTag {
        Marking,
        Tenuring,
        Callback
    };
    bool isMarkingTracer() const { return tag_ == TracerKindTag::Marking; }
    bool isTenuringTracer() const { return tag_ == TracerKindTag::Tenuring; }
    bool isCallbackTracer() const { return tag_ == TracerKindTag::Callback; }
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

class JS_PUBLIC_API(CallbackTracer) : public JSTracer
{
  public:
    CallbackTracer(JSRuntime* rt, WeakMapTraceKind weakTraceKind = TraceWeakMapValues)
      : JSTracer(rt, JSTracer::TracerKindTag::Callback, weakTraceKind),
        contextName_(nullptr), contextIndex_(InvalidIndex), contextFunctor_(nullptr)
    {}

    
    
    
    
    
    virtual void onObjectEdge(JSObject** objp) { onChild(JS::GCCellPtr(*objp)); }
    virtual void onStringEdge(JSString** strp) { onChild(JS::GCCellPtr(*strp)); }
    virtual void onSymbolEdge(JS::Symbol** symp) { onChild(JS::GCCellPtr(*symp)); }
    virtual void onScriptEdge(JSScript** scriptp) { onChild(JS::GCCellPtr(*scriptp)); }
    virtual void onShapeEdge(js::Shape** shapep) {
        onChild(JS::GCCellPtr(*shapep, JS::TraceKind::Shape));
    }
    virtual void onObjectGroupEdge(js::ObjectGroup** groupp) {
        onChild(JS::GCCellPtr(*groupp, JS::TraceKind::ObjectGroup));
    }
    virtual void onBaseShapeEdge(js::BaseShape** basep) {
        onChild(JS::GCCellPtr(*basep, JS::TraceKind::BaseShape));
    }
    virtual void onJitCodeEdge(js::jit::JitCode** codep) {
        onChild(JS::GCCellPtr(*codep, JS::TraceKind::JitCode));
    }
    virtual void onLazyScriptEdge(js::LazyScript** lazyp) {
        onChild(JS::GCCellPtr(*lazyp, JS::TraceKind::LazyScript));
    }

    
    
    virtual void onChild(const JS::GCCellPtr& thing) = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    const char* contextName() const { MOZ_ASSERT(contextName_); return contextName_; }

    
    
    
    const static size_t InvalidIndex = size_t(-1);
    size_t contextIndex() const { return contextIndex_; }

    
    
    
    
    
    void getTracingEdgeName(char* buffer, size_t bufferSize);

    
    
    
    
    
    class ContextFunctor {
      public:
        virtual void operator()(CallbackTracer* trc, char* buf, size_t bufsize) = 0;
    };

#ifdef DEBUG
    enum class TracerKind { DoNotCare, Moving, GrayBuffering, VerifyTraceProtoAndIface };
    virtual TracerKind getTracerKind() const { return TracerKind::DoNotCare; }
#endif






    void dispatchToOnEdge(JSObject** objp) { onObjectEdge(objp); }
    void dispatchToOnEdge(JSString** strp) { onStringEdge(strp); }
    void dispatchToOnEdge(JS::Symbol** symp) { onSymbolEdge(symp); }
    void dispatchToOnEdge(JSScript** scriptp) { onScriptEdge(scriptp); }
    void dispatchToOnEdge(js::Shape** shapep) { onShapeEdge(shapep); }
    void dispatchToOnEdge(js::ObjectGroup** groupp) { onObjectGroupEdge(groupp); }
    void dispatchToOnEdge(js::BaseShape** basep) { onBaseShapeEdge(basep); }
    void dispatchToOnEdge(js::jit::JitCode** codep) { onJitCodeEdge(codep); }
    void dispatchToOnEdge(js::LazyScript** lazyp) { onLazyScriptEdge(lazyp); }

  private:
    friend class AutoTracingName;
    const char* contextName_;

    friend class AutoTracingIndex;
    size_t contextIndex_;

    friend class AutoTracingDetails;
    ContextFunctor* contextFunctor_;
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
    JS_CallUnbarrieredObjectTracer(trc, &updated, name);
    if (updated != key)
        e.rekeyFront(updated);
}



extern JS_PUBLIC_API(void)
JS_CallTenuredObjectTracer(JSTracer* trc, JS::TenuredHeap<JSObject*>* objp, const char* name);

extern JS_PUBLIC_API(void)
JS_TraceChildren(JSTracer* trc, void* thing, JS::TraceKind kind);

extern JS_PUBLIC_API(void)
JS_TraceRuntime(JSTracer* trc);

namespace JS {
typedef js::HashSet<Zone*, js::DefaultHasher<Zone*>, js::SystemAllocPolicy> ZoneSet;
}



extern JS_PUBLIC_API(void)
JS_TraceIncomingCCWs(JSTracer* trc, const JS::ZoneSet& zones);

extern JS_PUBLIC_API(void)
JS_GetTraceThingInfo(char* buf, size_t bufsize, JSTracer* trc,
                     void* thing, JS::TraceKind kind, bool includeDetails);

#endif 
