





#ifndef js_HeapAPI_h
#define js_HeapAPI_h

#include <limits.h>

#include "jspubtd.h"

#include "js/Utility.h"


namespace js {



JS_FRIEND_API(bool)
CurrentThreadCanAccessRuntime(JSRuntime *rt);

JS_FRIEND_API(bool)
CurrentThreadCanAccessZone(JS::Zone *zone);

namespace gc {

struct Cell;

const size_t ArenaShift = 12;
const size_t ArenaSize = size_t(1) << ArenaShift;
const size_t ArenaMask = ArenaSize - 1;

#ifdef JS_GC_SMALL_CHUNK_SIZE
const size_t ChunkShift = 18;
#else
const size_t ChunkShift = 20;
#endif
const size_t ChunkSize = size_t(1) << ChunkShift;
const size_t ChunkMask = ChunkSize - 1;

const size_t CellShift = 3;
const size_t CellSize = size_t(1) << CellShift;
const size_t CellMask = CellSize - 1;


#ifdef JS_GC_SMALL_CHUNK_SIZE
const size_t ChunkMarkBitmapOffset = 258104;
const size_t ChunkMarkBitmapBits = 31744;
#else
const size_t ChunkMarkBitmapOffset = 1032352;
const size_t ChunkMarkBitmapBits = 129024;
#endif
const size_t ChunkRuntimeOffset = ChunkSize - sizeof(void*);
const size_t ChunkLocationOffset = ChunkSize - 2 * sizeof(void*) - sizeof(uint64_t);






static const uint32_t BLACK = 0;
static const uint32_t GRAY = 1;











const uintptr_t ChunkLocationBitNursery = 1;       
const uintptr_t ChunkLocationBitTenuredHeap = 2;   
const uintptr_t ChunkLocationBitPJSNewspace = 4;   
const uintptr_t ChunkLocationBitPJSFromspace = 8;  

const uintptr_t ChunkLocationAnyNursery = ChunkLocationBitNursery |
                                          ChunkLocationBitPJSNewspace |
                                          ChunkLocationBitPJSFromspace;

#ifdef JS_DEBUG

extern JS_FRIEND_API(void)
AssertGCThingHasType(js::gc::Cell *cell, JSGCTraceKind kind);
#else
inline void
AssertGCThingHasType(js::gc::Cell *cell, JSGCTraceKind kind) {}
#endif

} 
} 

namespace JS {
struct Zone;


const uint32_t DefaultNurseryBytes = 16 * 1024 * 1024;


const uint32_t DefaultHeapMaxBytes = 32 * 1024 * 1024;





static MOZ_ALWAYS_INLINE js::gc::Cell *
AsCell(JSObject *obj)
{
    js::gc::Cell *cell = reinterpret_cast<js::gc::Cell *>(obj);
    js::gc::AssertGCThingHasType(cell, JSTRACE_OBJECT);
    return cell;
}

static MOZ_ALWAYS_INLINE js::gc::Cell *
AsCell(JSFunction *fun)
{
    js::gc::Cell *cell = reinterpret_cast<js::gc::Cell *>(fun);
    js::gc::AssertGCThingHasType(cell, JSTRACE_OBJECT);
    return cell;
}

static MOZ_ALWAYS_INLINE js::gc::Cell *
AsCell(JSString *str)
{
    js::gc::Cell *cell = reinterpret_cast<js::gc::Cell *>(str);
    js::gc::AssertGCThingHasType(cell, JSTRACE_STRING);
    return cell;
}

static MOZ_ALWAYS_INLINE js::gc::Cell *
AsCell(JSFlatString *flat)
{
    js::gc::Cell *cell = reinterpret_cast<js::gc::Cell *>(flat);
    js::gc::AssertGCThingHasType(cell, JSTRACE_STRING);
    return cell;
}

static MOZ_ALWAYS_INLINE js::gc::Cell *
AsCell(JS::Symbol *sym)
{
    js::gc::Cell *cell = reinterpret_cast<js::gc::Cell *>(sym);
    js::gc::AssertGCThingHasType(cell, JSTRACE_SYMBOL);
    return cell;
}

static MOZ_ALWAYS_INLINE js::gc::Cell *
AsCell(JSScript *script)
{
    js::gc::Cell *cell = reinterpret_cast<js::gc::Cell *>(script);
    js::gc::AssertGCThingHasType(cell, JSTRACE_SCRIPT);
    return cell;
}

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
    bool needsIncrementalBarrier_;

    Zone(JSRuntime *runtime, JSTracer *barrierTracerArg)
      : runtime_(runtime),
        barrierTracer_(barrierTracerArg),
        needsIncrementalBarrier_(false)
    {}

    bool needsIncrementalBarrier() const {
        return needsIncrementalBarrier_;
    }

    JSTracer *barrierTracer() {
        MOZ_ASSERT(needsIncrementalBarrier_);
        MOZ_ASSERT(js::CurrentThreadCanAccessRuntime(runtime_));
        return barrierTracer_;
    }

    JSRuntime *runtimeFromMainThread() const {
        MOZ_ASSERT(js::CurrentThreadCanAccessRuntime(runtime_));
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

static MOZ_ALWAYS_INLINE uintptr_t *
GetGCThingMarkBitmap(const void *thing)
{
    MOZ_ASSERT(thing);
    uintptr_t addr = uintptr_t(thing);
    addr &= ~js::gc::ChunkMask;
    addr |= js::gc::ChunkMarkBitmapOffset;
    return reinterpret_cast<uintptr_t *>(addr);
}

static MOZ_ALWAYS_INLINE JS::shadow::Runtime *
GetGCThingRuntime(const void *thing)
{
    MOZ_ASSERT(thing);
    uintptr_t addr = uintptr_t(thing);
    addr &= ~js::gc::ChunkMask;
    addr |= js::gc::ChunkRuntimeOffset;
    return *reinterpret_cast<JS::shadow::Runtime **>(addr);
}

static MOZ_ALWAYS_INLINE void
GetGCThingMarkWordAndMask(const void *thing, uint32_t color,
                          uintptr_t **wordp, uintptr_t *maskp)
{
    uintptr_t addr = uintptr_t(thing);
    size_t bit = (addr & js::gc::ChunkMask) / js::gc::CellSize + color;
    MOZ_ASSERT(bit < js::gc::ChunkMarkBitmapBits);
    uintptr_t *bitmap = GetGCThingMarkBitmap(thing);
    const uintptr_t nbits = sizeof(*bitmap) * CHAR_BIT;
    *maskp = uintptr_t(1) << (bit % nbits);
    *wordp = &bitmap[bit / nbits];
}

static MOZ_ALWAYS_INLINE JS::shadow::ArenaHeader *
GetGCThingArena(void *thing)
{
    uintptr_t addr = uintptr_t(thing);
    addr &= ~js::gc::ArenaMask;
    return reinterpret_cast<JS::shadow::ArenaHeader *>(addr);
}

MOZ_ALWAYS_INLINE bool
IsInsideNursery(const js::gc::Cell *cell)
{
#ifdef JSGC_GENERATIONAL
    if (!cell)
        return false;
    uintptr_t addr = uintptr_t(cell);
    addr &= ~js::gc::ChunkMask;
    addr |= js::gc::ChunkLocationOffset;
    uint32_t location = *reinterpret_cast<uint32_t *>(addr);
    JS_ASSERT(location != 0);
    return location & ChunkLocationAnyNursery;
#else
    return false;
#endif
}

} 

} 

namespace JS {

static MOZ_ALWAYS_INLINE Zone *
GetTenuredGCThingZone(void *thing)
{
    MOZ_ASSERT(thing);
#ifdef JSGC_GENERATIONAL
    JS_ASSERT(!js::gc::IsInsideNursery((js::gc::Cell *)thing));
#endif
    return js::gc::GetGCThingArena(thing)->zone;
}

extern JS_PUBLIC_API(Zone *)
GetObjectZone(JSObject *obj);

static MOZ_ALWAYS_INLINE bool
GCThingIsMarkedGray(void *thing)
{
    MOZ_ASSERT(thing);
#ifdef JSGC_GENERATIONAL
    




    if (js::gc::IsInsideNursery((js::gc::Cell *)thing))
        return false;
#endif
    uintptr_t *word, mask;
    js::gc::GetGCThingMarkWordAndMask(thing, js::gc::GRAY, &word, &mask);
    return *word & mask;
}

static MOZ_ALWAYS_INLINE bool
IsIncrementalBarrierNeededOnTenuredGCThing(shadow::Runtime *rt, void *thing, JSGCTraceKind kind)
{
    MOZ_ASSERT(thing);
#ifdef JSGC_GENERATIONAL
    MOZ_ASSERT(!js::gc::IsInsideNursery((js::gc::Cell *)thing));
#endif
    if (!rt->needsIncrementalBarrier())
        return false;
    JS::Zone *zone = GetTenuredGCThingZone(thing);
    return reinterpret_cast<shadow::Zone *>(zone)->needsIncrementalBarrier();
}

} 

#endif 
