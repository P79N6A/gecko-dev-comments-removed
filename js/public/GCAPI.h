





#ifndef js_GCAPI_h
#define js_GCAPI_h

#include "mozilla/NullPtr.h"

#include "js/HeapAPI.h"

namespace js {
namespace gc {
class GCRuntime;
}
}

typedef enum JSGCMode {
    
    JSGC_MODE_GLOBAL = 0,

    
    JSGC_MODE_COMPARTMENT = 1,

    



    JSGC_MODE_INCREMENTAL = 2
} JSGCMode;

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
    D(COMPARTMENT_REVIVED)                      \
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
    D(FINISH_LARGE_EVALUATE)

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

    explicit GCDescription(bool isCompartment)
      : isCompartment_(isCompartment) {}

    char16_t *formatMessage(JSRuntime *rt) const;
    char16_t *formatJSON(JSRuntime *rt, uint64_t timestamp) const;
};

typedef void
(* GCSliceCallback)(JSRuntime *rt, GCProgress progress, const GCDescription &desc);






extern JS_FRIEND_API(GCSliceCallback)
SetGCSliceCallback(JSRuntime *rt, GCSliceCallback callback);







extern JS_FRIEND_API(void)
DisableIncrementalGC(JSRuntime *rt);









extern JS_FRIEND_API(bool)
IsIncrementalGCEnabled(JSRuntime *rt);





JS_FRIEND_API(bool)
IsIncrementalGCInProgress(JSRuntime *rt);






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




extern JS_FRIEND_API(bool)
WasIncrementalGC(JSRuntime *rt);










class JS_FRIEND_API(AutoDisableGenerationalGC)
{
    js::gc::GCRuntime *gc;
#if defined(JSGC_GENERATIONAL) && defined(JS_GC_ZEAL)
    bool restartVerifier;
#endif

  public:
    explicit AutoDisableGenerationalGC(JSRuntime *rt);
    ~AutoDisableGenerationalGC();
};





extern JS_FRIEND_API(bool)
IsGenerationalGCEnabled(JSRuntime *rt);






extern JS_FRIEND_API(size_t)
GetGCNumber();






extern JS_FRIEND_API(void)
ShrinkGCBuffers(JSRuntime *rt);





class JS_PUBLIC_API(AutoAssertOnGC)
{
#ifdef DEBUG
    js::gc::GCRuntime *gc;
    size_t gcNumber;

  public:
    AutoAssertOnGC();
    explicit AutoAssertOnGC(JSRuntime *rt);
    ~AutoAssertOnGC();

    static void VerifyIsSafeToGC(JSRuntime *rt);
#else
  public:
    AutoAssertOnGC() {}
    explicit AutoAssertOnGC(JSRuntime *rt) {}
    ~AutoAssertOnGC() {}

    static void VerifyIsSafeToGC(JSRuntime *rt) {}
#endif
};





class JS_PUBLIC_API(AutoAssertNoAlloc)
{
#ifdef JS_DEBUG
    js::gc::GCRuntime *gc;

  public:
    AutoAssertNoAlloc() : gc(nullptr) {}
    explicit AutoAssertNoAlloc(JSRuntime *rt);
    void disallowAlloc(JSRuntime *rt);
    ~AutoAssertNoAlloc();
#else
  public:
    AutoAssertNoAlloc() {}
    explicit AutoAssertNoAlloc(JSRuntime *rt) {}
    void disallowAlloc(JSRuntime *rt) {}
#endif
};















class JS_PUBLIC_API(AutoSuppressGCAnalysis) : public AutoAssertNoAlloc
{
  public:
    AutoSuppressGCAnalysis() : AutoAssertNoAlloc() {}
    explicit AutoSuppressGCAnalysis(JSRuntime *rt) : AutoAssertNoAlloc(rt) {}
};









class JS_PUBLIC_API(AutoAssertGCCallback) : public AutoSuppressGCAnalysis
{
  public:
    explicit AutoAssertGCCallback(JSObject *obj);
};











class JS_PUBLIC_API(AutoCheckCannotGC) : public AutoAssertOnGC
{
  public:
    AutoCheckCannotGC() : AutoAssertOnGC() {}
    explicit AutoCheckCannotGC(JSRuntime *rt) : AutoAssertOnGC(rt) {}
};





extern JS_FRIEND_API(bool)
UnmarkGrayGCThingRecursively(void *thing, JSGCTraceKind kind);

} 

namespace js {
namespace gc {

static MOZ_ALWAYS_INLINE void
ExposeGCThingToActiveJS(void *thing, JSGCTraceKind kind)
{
    MOZ_ASSERT(kind != JSTRACE_SHAPE);

    JS::shadow::Runtime *rt = GetGCThingRuntime(thing);
#ifdef JSGC_GENERATIONAL
    




    if (IsInsideNursery((Cell *)thing))
        return;
#endif
    if (JS::IsIncrementalBarrierNeededOnTenuredGCThing(rt, thing, kind))
        JS::IncrementalReferenceBarrier(thing, kind);
    else if (JS::GCThingIsMarkedGray(thing))
        JS::UnmarkGrayGCThingRecursively(thing, kind);
}

} 
} 

namespace JS {







static MOZ_ALWAYS_INLINE void
ExposeObjectToActiveJS(JSObject *obj)
{
    js::gc::ExposeGCThingToActiveJS(obj, JSTRACE_OBJECT);
}

static MOZ_ALWAYS_INLINE void
ExposeScriptToActiveJS(JSScript *script)
{
    js::gc::ExposeGCThingToActiveJS(script, JSTRACE_SCRIPT);
}




static MOZ_ALWAYS_INLINE void
MarkGCThingAsLive(JSRuntime *rt_, void *thing, JSGCTraceKind kind)
{
    shadow::Runtime *rt = shadow::Runtime::asShadowRuntime(rt_);
#ifdef JSGC_GENERATIONAL
    


    if (js::gc::IsInsideNursery((js::gc::Cell *)thing))
        return;
#endif
    if (IsIncrementalBarrierNeededOnTenuredGCThing(rt, thing, kind))
        IncrementalReferenceBarrier(thing, kind);
}

static MOZ_ALWAYS_INLINE void
MarkStringAsLive(Zone *zone, JSString *string)
{
    JSRuntime *rt = JS::shadow::Zone::asShadowZone(zone)->runtimeFromMainThread();
    MarkGCThingAsLive(rt, string, JSTRACE_STRING);
}






extern JS_FRIEND_API(void)
PokeGC(JSRuntime *rt);




extern JS_FRIEND_API(void)
NotifyDidPaint(JSRuntime *rt);

} 

#endif 
