





#ifndef js_GCAPI_h
#define js_GCAPI_h

#include "mozilla/UniquePtr.h"
#include "mozilla/Vector.h"

#include "js/HeapAPI.h"

namespace js {
namespace gc {
class GCRuntime;
}
namespace gcstats {
struct Statistics;
}
}

typedef enum JSGCMode {
    
    JSGC_MODE_GLOBAL = 0,

    
    JSGC_MODE_COMPARTMENT = 1,

    



    JSGC_MODE_INCREMENTAL = 2
} JSGCMode;




typedef enum JSGCInvocationKind {
    
    GC_NORMAL = 0,

    
    GC_SHRINK = 1
} JSGCInvocationKind;

namespace JS {

using mozilla::UniquePtr;

#define GCREASONS(D)                            \
    /* Reasons internal to the JS engine */     \
    D(API)                                      \
    D(EAGER_ALLOC_TRIGGER)                      \
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
    D(SHARED_MEMORY_LIMIT)                      \
    D(PERIODIC_FULL_GC)                         \
    D(INCREMENTAL_TOO_SLOW)                     \
    D(ABORT_GC)                                 \
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
    D(FINISH_LARGE_EVALUATE)                    \
    D(USER_INACTIVE)

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


















extern JS_PUBLIC_API(void)
PrepareZoneForGC(Zone* zone);




extern JS_PUBLIC_API(void)
PrepareForFullGC(JSRuntime* rt);






extern JS_PUBLIC_API(void)
PrepareForIncrementalGC(JSRuntime* rt);





extern JS_PUBLIC_API(bool)
IsGCScheduled(JSRuntime* rt);





extern JS_PUBLIC_API(void)
SkipZoneForGC(Zone* zone);
















extern JS_PUBLIC_API(void)
GCForReason(JSRuntime* rt, JSGCInvocationKind gckind, gcreason::Reason reason);

































extern JS_PUBLIC_API(void)
StartIncrementalGC(JSRuntime* rt, JSGCInvocationKind gckind, gcreason::Reason reason,
                   int64_t millis = 0);









extern JS_PUBLIC_API(void)
IncrementalGCSlice(JSRuntime* rt, gcreason::Reason reason, int64_t millis = 0);







extern JS_PUBLIC_API(void)
FinishIncrementalGC(JSRuntime* rt, gcreason::Reason reason);







extern JS_PUBLIC_API(void)
AbortIncrementalGC(JSRuntime* rt);

namespace dbg {




class GarbageCollectionEvent
{
    
    uint64_t majorGCNumber_;

    
    
    const char* reason;

    
    
    
    const char* nonincrementalReason;

    
    
    struct Collection {
        int64_t startTimestamp;
        int64_t endTimestamp;
    };

    
    mozilla::Vector<Collection> collections;

    GarbageCollectionEvent(const GarbageCollectionEvent& rhs) = delete;
    GarbageCollectionEvent& operator=(const GarbageCollectionEvent& rhs) = delete;

  public:
    explicit GarbageCollectionEvent(uint64_t majorGCNum)
        : majorGCNumber_(majorGCNum)
        , reason(nullptr)
        , nonincrementalReason(nullptr)
        , collections()
    { }

    using Ptr = UniquePtr<GarbageCollectionEvent, DeletePolicy<GarbageCollectionEvent>>;
    static Ptr Create(JSRuntime* rt, ::js::gcstats::Statistics& stats, uint64_t majorGCNumber);

    JSObject* toJSObject(JSContext* cx) const;

    uint64_t majorGCNumber() const { return majorGCNumber_; }
};

} 

enum GCProgress {
    









    GC_CYCLE_BEGIN,
    GC_SLICE_BEGIN,
    GC_SLICE_END,
    GC_CYCLE_END
};

struct JS_PUBLIC_API(GCDescription) {
    bool isCompartment_;
    JSGCInvocationKind invocationKind_;

    GCDescription(bool isCompartment, JSGCInvocationKind kind)
      : isCompartment_(isCompartment), invocationKind_(kind) {}

    char16_t* formatMessage(JSRuntime* rt) const;
    char16_t* formatJSON(JSRuntime* rt, uint64_t timestamp) const;

    JS::dbg::GarbageCollectionEvent::Ptr toGCEvent(JSRuntime* rt) const;
};

typedef void
(* GCSliceCallback)(JSRuntime* rt, GCProgress progress, const GCDescription& desc);






extern JS_PUBLIC_API(GCSliceCallback)
SetGCSliceCallback(JSRuntime* rt, GCSliceCallback callback);







extern JS_PUBLIC_API(void)
DisableIncrementalGC(JSRuntime* rt);









extern JS_PUBLIC_API(bool)
IsIncrementalGCEnabled(JSRuntime* rt);





extern JS_PUBLIC_API(bool)
IsIncrementalGCInProgress(JSRuntime* rt);






extern JS_PUBLIC_API(bool)
IsIncrementalBarrierNeeded(JSRuntime* rt);

extern JS_PUBLIC_API(bool)
IsIncrementalBarrierNeeded(JSContext* cx);





extern JS_PUBLIC_API(void)
IncrementalReferenceBarrier(GCCellPtr thing);

extern JS_PUBLIC_API(void)
IncrementalValueBarrier(const Value& v);

extern JS_PUBLIC_API(void)
IncrementalObjectBarrier(JSObject* obj);




extern JS_PUBLIC_API(bool)
WasIncrementalGC(JSRuntime* rt);










class JS_PUBLIC_API(AutoDisableGenerationalGC)
{
    js::gc::GCRuntime* gc;
#ifdef JS_GC_ZEAL
    bool restartVerifier;
#endif

  public:
    explicit AutoDisableGenerationalGC(JSRuntime* rt);
    ~AutoDisableGenerationalGC();
};





extern JS_PUBLIC_API(bool)
IsGenerationalGCEnabled(JSRuntime* rt);






extern JS_PUBLIC_API(size_t)
GetGCNumber();






extern JS_PUBLIC_API(void)
ShrinkGCBuffers(JSRuntime* rt);





class JS_PUBLIC_API(AutoAssertOnGC)
{
#ifdef DEBUG
    js::gc::GCRuntime* gc;
    size_t gcNumber;

  public:
    AutoAssertOnGC();
    explicit AutoAssertOnGC(JSRuntime* rt);
    ~AutoAssertOnGC();

    static void VerifyIsSafeToGC(JSRuntime* rt);
#else
  public:
    AutoAssertOnGC() {}
    explicit AutoAssertOnGC(JSRuntime* rt) {}
    ~AutoAssertOnGC() {}

    static void VerifyIsSafeToGC(JSRuntime* rt) {}
#endif
};





class JS_PUBLIC_API(AutoAssertNoAlloc)
{
#ifdef JS_DEBUG
    js::gc::GCRuntime* gc;

  public:
    AutoAssertNoAlloc() : gc(nullptr) {}
    explicit AutoAssertNoAlloc(JSRuntime* rt);
    void disallowAlloc(JSRuntime* rt);
    ~AutoAssertNoAlloc();
#else
  public:
    AutoAssertNoAlloc() {}
    explicit AutoAssertNoAlloc(JSRuntime* rt) {}
    void disallowAlloc(JSRuntime* rt) {}
#endif
};















class JS_PUBLIC_API(AutoSuppressGCAnalysis) : public AutoAssertNoAlloc
{
  public:
    AutoSuppressGCAnalysis() : AutoAssertNoAlloc() {}
    explicit AutoSuppressGCAnalysis(JSRuntime* rt) : AutoAssertNoAlloc(rt) {}
};









class JS_PUBLIC_API(AutoAssertGCCallback) : public AutoSuppressGCAnalysis
{
  public:
    explicit AutoAssertGCCallback(JSObject* obj);
};











class JS_PUBLIC_API(AutoCheckCannotGC) : public AutoAssertOnGC
{
  public:
    AutoCheckCannotGC() : AutoAssertOnGC() {}
    explicit AutoCheckCannotGC(JSRuntime* rt) : AutoAssertOnGC(rt) {}
};





extern JS_FRIEND_API(bool)
UnmarkGrayGCThingRecursively(GCCellPtr thing);

} 

namespace js {
namespace gc {

static MOZ_ALWAYS_INLINE void
ExposeGCThingToActiveJS(JS::GCCellPtr thing)
{
    MOZ_ASSERT(thing.kind() != JSTRACE_SHAPE);

    




    if (IsInsideNursery(thing.asCell()))
        return;
    JS::shadow::Runtime* rt = detail::GetGCThingRuntime(thing.unsafeAsUIntPtr());
    if (IsIncrementalBarrierNeededOnTenuredGCThing(rt, thing))
        JS::IncrementalReferenceBarrier(thing);
    else if (JS::GCThingIsMarkedGray(thing))
        JS::UnmarkGrayGCThingRecursively(thing);
}

static MOZ_ALWAYS_INLINE void
MarkGCThingAsLive(JSRuntime* aRt, JS::GCCellPtr thing)
{
    JS::shadow::Runtime* rt = JS::shadow::Runtime::asShadowRuntime(aRt);
    


    if (IsInsideNursery(thing.asCell()))
        return;
    if (IsIncrementalBarrierNeededOnTenuredGCThing(rt, thing))
        JS::IncrementalReferenceBarrier(thing);
}

} 
} 

namespace JS {







static MOZ_ALWAYS_INLINE void
ExposeObjectToActiveJS(JSObject* obj)
{
    js::gc::ExposeGCThingToActiveJS(GCCellPtr(obj));
}

static MOZ_ALWAYS_INLINE void
ExposeScriptToActiveJS(JSScript* script)
{
    js::gc::ExposeGCThingToActiveJS(GCCellPtr(script));
}




static MOZ_ALWAYS_INLINE void
MarkStringAsLive(Zone* zone, JSString* string)
{
    JSRuntime* rt = JS::shadow::Zone::asShadowZone(zone)->runtimeFromMainThread();
    js::gc::MarkGCThingAsLive(rt, GCCellPtr(string));
}






extern JS_FRIEND_API(void)
PokeGC(JSRuntime* rt);




extern JS_FRIEND_API(void)
NotifyDidPaint(JSRuntime* rt);

} 

#endif 
