





#ifndef js_GCAPI_h
#define js_GCAPI_h

#include "mozilla/NullPtr.h"

#include "js/HeapAPI.h"
#include "js/RootingAPI.h"
#include "js/Value.h"

namespace JS {

#define GCREASONS(D)                            \
    /* Reasons internal to the JS engine */     \
    D(API)                                      \
    D(MAYBEGC)                                  \
    D(DESTROY_RUNTIME)                          \
    D(DESTROY_CONTEXT)                          \
    D(LAST_DITCH)                               \
    D(TOO_MUCH_MALLOC)                          \
    D(ALLOC_TRIGGER)                            \
    D(DEBUG_GC)                                 \
    D(DEBUG_MODE_GC)                            \
    D(TRANSPLANT)                               \
    D(RESET)                                    \
    D(OUT_OF_NURSERY)                           \
    D(EVICT_NURSERY)                            \
    D(FULL_STORE_BUFFER)                        \
                                                \
    /* These are reserved for future use. */    \
    D(RESERVED0)                                \
    D(RESERVED1)                                \
    D(RESERVED2)                                \
    D(RESERVED3)                                \
    D(RESERVED4)                                \
    D(RESERVED5)                                \
    D(RESERVED6)                                \
    D(RESERVED7)                                \
    D(RESERVED8)                                \
    D(RESERVED9)                                \
    D(RESERVED10)                               \
    D(RESERVED11)                               \
    D(RESERVED12)                               \
    D(RESERVED13)                               \
    D(RESERVED14)                               \
    D(RESERVED15)                               \
    D(RESERVED16)                               \
    D(RESERVED17)                               \
    D(RESERVED18)                               \
    D(RESERVED19)                               \
                                                \
    /* Reasons from Firefox */                  \
    D(DOM_WINDOW_UTILS)                         \
    D(COMPONENT_UTILS)                          \
    D(MEM_PRESSURE)                             \
    D(CC_WAITING)                               \
    D(CC_FORCED)                                \
    D(LOAD_END)                                 \
    D(POST_COMPARTMENT)                         \
    D(PAGE_HIDE)                                \
    D(NSJSCONTEXT_DESTROY)                      \
    D(SET_NEW_DOCUMENT)                         \
    D(SET_DOC_SHELL)                            \
    D(DOM_UTILS)                                \
    D(DOM_IPC)                                  \
    D(DOM_WORKER)                               \
    D(INTER_SLICE_GC)                           \
    D(REFRESH_FRAME)                            \
    D(FULL_GC_TIMER)                            \
    D(SHUTDOWN_CC)                              \
    D(FINISH_LARGE_EVALUTE)

namespace gcreason {


enum Reason {
#define MAKE_REASON(name) name,
    GCREASONS(MAKE_REASON)
#undef MAKE_REASON
    NO_REASON,
    NUM_REASONS,

    





    NUM_TELEMETRY_REASONS = 100
};

} 

extern JS_FRIEND_API(void)
PrepareZoneForGC(Zone *zone);

extern JS_FRIEND_API(void)
PrepareForFullGC(JSRuntime *rt);

extern JS_FRIEND_API(void)
PrepareForIncrementalGC(JSRuntime *rt);

extern JS_FRIEND_API(bool)
IsGCScheduled(JSRuntime *rt);

extern JS_FRIEND_API(void)
SkipZoneForGC(Zone *zone);








extern JS_FRIEND_API(void)
GCForReason(JSRuntime *rt, gcreason::Reason reason);

extern JS_FRIEND_API(void)
ShrinkingGC(JSRuntime *rt, gcreason::Reason reason);

extern JS_FRIEND_API(void)
ShrinkGCBuffers(JSRuntime *rt);

extern JS_FRIEND_API(void)
IncrementalGC(JSRuntime *rt, gcreason::Reason reason, int64_t millis = 0);

extern JS_FRIEND_API(void)
FinishIncrementalGC(JSRuntime *rt, gcreason::Reason reason);

enum GCProgress {
    









    GC_CYCLE_BEGIN,
    GC_SLICE_BEGIN,
    GC_SLICE_END,
    GC_CYCLE_END
};

struct JS_FRIEND_API(GCDescription) {
    bool isCompartment_;

    GCDescription(bool isCompartment)
      : isCompartment_(isCompartment) {}

    jschar *formatMessage(JSRuntime *rt) const;
    jschar *formatJSON(JSRuntime *rt, uint64_t timestamp) const;
};

typedef void
(* GCSliceCallback)(JSRuntime *rt, GCProgress progress, const GCDescription &desc);

extern JS_FRIEND_API(GCSliceCallback)
SetGCSliceCallback(JSRuntime *rt, GCSliceCallback callback);





extern JS_FRIEND_API(void)
NotifyDidPaint(JSRuntime *rt);

extern JS_FRIEND_API(bool)
IsIncrementalGCEnabled(JSRuntime *rt);

JS_FRIEND_API(bool)
IsIncrementalGCInProgress(JSRuntime *rt);

extern JS_FRIEND_API(void)
DisableIncrementalGC(JSRuntime *rt);

extern JS_FRIEND_API(void)
DisableGenerationalGC(JSRuntime *rt);

extern JS_FRIEND_API(void)
EnableGenerationalGC(JSRuntime *rt);

extern JS_FRIEND_API(bool)
IsIncrementalBarrierNeeded(JSRuntime *rt);

extern JS_FRIEND_API(bool)
IsIncrementalBarrierNeeded(JSContext *cx);

extern JS_FRIEND_API(void)
IncrementalReferenceBarrier(void *ptr, JSGCTraceKind kind);

extern JS_FRIEND_API(void)
IncrementalValueBarrier(const Value &v);

extern JS_FRIEND_API(void)
IncrementalObjectBarrier(JSObject *obj);

extern JS_FRIEND_API(void)
PokeGC(JSRuntime *rt);


extern JS_FRIEND_API(bool)
WasIncrementalGC(JSRuntime *rt);

extern JS_FRIEND_API(size_t)
GetGCNumber();

class JS_PUBLIC_API(AutoAssertNoGC)
{
#ifdef DEBUG
    JSRuntime *runtime;
    size_t gcNumber;

  public:
    AutoAssertNoGC();
    AutoAssertNoGC(JSRuntime *rt);
    ~AutoAssertNoGC();
#else
  public:
    
    AutoAssertNoGC() {}
    AutoAssertNoGC(JSRuntime *) {}
#endif
};

class JS_PUBLIC_API(ObjectPtr)
{
    Heap<JSObject *> value;

  public:
    ObjectPtr() : value(nullptr) {}

    ObjectPtr(JSObject *obj) : value(obj) {}

    
    ~ObjectPtr() { JS_ASSERT(!value); }

    void finalize(JSRuntime *rt) {
        if (IsIncrementalBarrierNeeded(rt))
            IncrementalObjectBarrier(value);
        value = nullptr;
    }

    void init(JSObject *obj) { value = obj; }

    JSObject *get() const { return value; }

    void writeBarrierPre(JSRuntime *rt) {
        IncrementalObjectBarrier(value);
    }

    bool isAboutToBeFinalized();

    ObjectPtr &operator=(JSObject *obj) {
        IncrementalObjectBarrier(value);
        value = obj;
        return *this;
    }

    void trace(JSTracer *trc, const char *name);

    JSObject &operator*() const { return *value; }
    JSObject *operator->() const { return value; }
    operator JSObject *() const { return value; }
};





extern JS_FRIEND_API(bool)
UnmarkGrayGCThingRecursively(void *thing, JSGCTraceKind kind);







static JS_ALWAYS_INLINE void
ExposeGCThingToActiveJS(void *thing, JSGCTraceKind kind)
{
    JS_ASSERT(kind != JSTRACE_SHAPE);

    shadow::Runtime *rt = js::gc::GetGCThingRuntime(thing);
#ifdef JSGC_GENERATIONAL
    




    if (js::gc::IsInsideNursery(rt, thing))
        return;
#endif
    if (IsIncrementalBarrierNeededOnGCThing(rt, thing, kind))
        IncrementalReferenceBarrier(thing, kind);
    else if (GCThingIsMarkedGray(thing))
        UnmarkGrayGCThingRecursively(thing, kind);
}

static JS_ALWAYS_INLINE void
ExposeValueToActiveJS(const Value &v)
{
    if (v.isMarkable())
        ExposeGCThingToActiveJS(v.toGCThing(), v.gcKind());
}

static JS_ALWAYS_INLINE void
ExposeObjectToActiveJS(JSObject *obj)
{
    ExposeGCThingToActiveJS(obj, JSTRACE_OBJECT);
}




static JS_ALWAYS_INLINE void
MarkGCThingAsLive(JSRuntime *rt_, void *thing, JSGCTraceKind kind)
{
    shadow::Runtime *rt = shadow::Runtime::asShadowRuntime(rt_);
#ifdef JSGC_GENERATIONAL
    


    if (js::gc::IsInsideNursery(rt, thing))
        return;
#endif
    if (IsIncrementalBarrierNeededOnGCThing(rt, thing, kind))
        IncrementalReferenceBarrier(thing, kind);
}

static JS_ALWAYS_INLINE void
MarkStringAsLive(Zone *zone, JSString *string)
{
    JSRuntime *rt = JS::shadow::Zone::asShadowZone(zone)->runtimeFromMainThread();
    MarkGCThingAsLive(rt, string, JSTRACE_STRING);
}

} 

#endif 
