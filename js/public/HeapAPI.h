





#ifndef js_HeapAPI_h
#define js_HeapAPI_h

#include <limits.h>

#include "jspubtd.h"

#include "js/Utility.h"


namespace js {



extern bool CurrentThreadCanAccessRuntime(JSRuntime *rt);
extern bool CurrentThreadCanAccessZone(JS::Zone *zone);

namespace gc {

const size_t ArenaShift = 12;
const size_t ArenaSize = size_t(1) << ArenaShift;
const size_t ArenaMask = ArenaSize - 1;

const size_t ChunkShift = 20;
const size_t ChunkSize = size_t(1) << ChunkShift;
const size_t ChunkMask = ChunkSize - 1;

const size_t CellShift = 3;
const size_t CellSize = size_t(1) << CellShift;
const size_t CellMask = CellSize - 1;


const size_t ChunkMarkBitmapOffset = 1032368;
const size_t ChunkMarkBitmapBits = 129024;
const size_t ChunkRuntimeOffset = ChunkSize - sizeof(void*);






static const uint32_t BLACK = 0;
static const uint32_t GRAY = 1;

} 
} 

namespace JS {
struct Zone;
} 

namespace JS {
namespace shadow {

struct ArenaHeader
{
    JS::Zone *zone;
};

struct Zone
{
  protected:
    JSRuntime *const runtime_;
    JSTracer *const barrierTracer_;     

  public:
    bool needsBarrier_;

    Zone(JSRuntime *runtime, JSTracer *barrierTracerArg)
      : runtime_(runtime),
        barrierTracer_(barrierTracerArg),
        needsBarrier_(false)
    {}

    bool needsBarrier() const {
        return needsBarrier_;
    }

    JSTracer *barrierTracer() {
        JS_ASSERT(needsBarrier_);
        JS_ASSERT(js::CurrentThreadCanAccessRuntime(runtime_));
        return barrierTracer_;
    }

    JSRuntime *runtimeFromMainThread() const {
        JS_ASSERT(js::CurrentThreadCanAccessRuntime(runtime_));
        return runtime_;
    }

    
    
    JSRuntime *runtimeFromAnyThread() const {
        return runtime_;
    }

    static JS::shadow::Zone *asShadowZone(JS::Zone *zone) {
        return reinterpret_cast<JS::shadow::Zone*>(zone);
    }
};

} 
} 

namespace js {
namespace gc {

static JS_ALWAYS_INLINE uintptr_t *
GetGCThingMarkBitmap(const void *thing)
{
    uintptr_t addr = uintptr_t(thing);
    addr &= ~js::gc::ChunkMask;
    addr |= js::gc::ChunkMarkBitmapOffset;
    return reinterpret_cast<uintptr_t *>(addr);
}

static JS_ALWAYS_INLINE JS::shadow::Runtime *
GetGCThingRuntime(const void *thing)
{
    uintptr_t addr = uintptr_t(thing);
    addr &= ~js::gc::ChunkMask;
    addr |= js::gc::ChunkRuntimeOffset;
    return *reinterpret_cast<JS::shadow::Runtime **>(addr);
}

static JS_ALWAYS_INLINE void
GetGCThingMarkWordAndMask(const void *thing, uint32_t color,
                          uintptr_t **wordp, uintptr_t *maskp)
{
    uintptr_t addr = uintptr_t(thing);
    size_t bit = (addr & js::gc::ChunkMask) / js::gc::CellSize + color;
    JS_ASSERT(bit < js::gc::ChunkMarkBitmapBits);
    uintptr_t *bitmap = GetGCThingMarkBitmap(thing);
    const uintptr_t nbits = sizeof(*bitmap) * CHAR_BIT;
    *maskp = uintptr_t(1) << (bit % nbits);
    *wordp = &bitmap[bit / nbits];
}

static JS_ALWAYS_INLINE JS::shadow::ArenaHeader *
GetGCThingArena(void *thing)
{
    uintptr_t addr = uintptr_t(thing);
    addr &= ~js::gc::ArenaMask;
    return reinterpret_cast<JS::shadow::ArenaHeader *>(addr);
}

} 

} 

namespace JS {

static JS_ALWAYS_INLINE Zone *
GetGCThingZone(void *thing)
{
    JS_ASSERT(thing);
    return js::gc::GetGCThingArena(thing)->zone;
}

static JS_ALWAYS_INLINE Zone *
GetObjectZone(JSObject *obj)
{
    return GetGCThingZone(obj);
}

static JS_ALWAYS_INLINE bool
GCThingIsMarkedGray(void *thing)
{
#ifdef JSGC_GENERATIONAL
    




    JS::shadow::Runtime *rt = js::gc::GetGCThingRuntime(thing);
    if (uintptr_t(thing) >= rt->gcNurseryStart_ && uintptr_t(thing) < rt->gcNurseryEnd_)
        return false;
#endif
    uintptr_t *word, mask;
    js::gc::GetGCThingMarkWordAndMask(thing, js::gc::GRAY, &word, &mask);
    return *word & mask;
}

static JS_ALWAYS_INLINE bool
IsIncrementalBarrierNeededOnGCThing(shadow::Runtime *rt, void *thing, JSGCTraceKind kind)
{
    if (!rt->needsBarrier_)
        return false;
    JS::Zone *zone = GetGCThingZone(thing);
    return reinterpret_cast<shadow::Zone *>(zone)->needsBarrier_;
}

} 

#endif 
