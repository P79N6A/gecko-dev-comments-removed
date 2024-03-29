





#ifndef js_HeapAPI_h
#define js_HeapAPI_h

#include <limits.h>

#include "jspubtd.h"

#include "js/TraceKind.h"
#include "js/Utility.h"


namespace js {



JS_FRIEND_API(bool)
CurrentThreadCanAccessRuntime(JSRuntime* rt);

JS_FRIEND_API(bool)
CurrentThreadCanAccessZone(JS::Zone* zone);

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
const size_t ArenaZoneOffset = 0;






static const uint32_t BLACK = 0;
static const uint32_t GRAY = 1;











const uintptr_t ChunkLocationBitNursery = 1;       
const uintptr_t ChunkLocationBitTenuredHeap = 2;   

const uintptr_t ChunkLocationAnyNursery = ChunkLocationBitNursery;

#ifdef JS_DEBUG

extern JS_FRIEND_API(void)
AssertGCThingHasType(js::gc::Cell* cell, JS::TraceKind kind);
#else
inline void
AssertGCThingHasType(js::gc::Cell* cell, JS::TraceKind kind) {}
#endif

MOZ_ALWAYS_INLINE bool IsInsideNursery(const js::gc::Cell* cell);

} 
} 

namespace JS {
struct Zone;


const uint32_t DefaultNurseryBytes = 16 * js::gc::ChunkSize;


const uint32_t DefaultHeapMaxBytes = 32 * 1024 * 1024;

namespace shadow {

struct Zone
{
  protected:
    JSRuntime* const runtime_;
    JSTracer* const barrierTracer_;     

  public:
    bool needsIncrementalBarrier_;

    Zone(JSRuntime* runtime, JSTracer* barrierTracerArg)
      : runtime_(runtime),
        barrierTracer_(barrierTracerArg),
        needsIncrementalBarrier_(false)
    {}

    bool needsIncrementalBarrier() const {
        return needsIncrementalBarrier_;
    }

    JSTracer* barrierTracer() {
        MOZ_ASSERT(needsIncrementalBarrier_);
        MOZ_ASSERT(js::CurrentThreadCanAccessRuntime(runtime_));
        return barrierTracer_;
    }

    JSRuntime* runtimeFromMainThread() const {
        MOZ_ASSERT(js::CurrentThreadCanAccessRuntime(runtime_));
        return runtime_;
    }

    
    
    JSRuntime* runtimeFromAnyThread() const {
        return runtime_;
    }

    static JS::shadow::Zone* asShadowZone(JS::Zone* zone) {
        return reinterpret_cast<JS::shadow::Zone*>(zone);
    }
};

} 






class JS_FRIEND_API(GCCellPtr)
{
  public:
    
    GCCellPtr(void* gcthing, JS::TraceKind traceKind) : ptr(checkedCast(gcthing, traceKind)) {}

    
    MOZ_IMPLICIT GCCellPtr(decltype(nullptr)) : ptr(checkedCast(nullptr, JS::TraceKind::Null)) {}

    
    explicit GCCellPtr(JSObject* obj) : ptr(checkedCast(obj, JS::TraceKind::Object)) { }
    explicit GCCellPtr(JSFunction* fun) : ptr(checkedCast(fun, JS::TraceKind::Object)) { }
    explicit GCCellPtr(JSString* str) : ptr(checkedCast(str, JS::TraceKind::String)) { }
    explicit GCCellPtr(JSFlatString* str) : ptr(checkedCast(str, JS::TraceKind::String)) { }
    explicit GCCellPtr(JS::Symbol* sym) : ptr(checkedCast(sym, JS::TraceKind::Symbol)) { }
    explicit GCCellPtr(JSScript* script) : ptr(checkedCast(script, JS::TraceKind::Script)) { }
    explicit GCCellPtr(const Value& v);

    JS::TraceKind kind() const {
        JS::TraceKind traceKind = JS::TraceKind(ptr & OutOfLineTraceKindMask);
        if (uintptr_t(traceKind) != OutOfLineTraceKindMask)
            return traceKind;
        return outOfLineKind();
    }

    
    explicit operator bool() const {
        MOZ_ASSERT(bool(asCell()) == (kind() != JS::TraceKind::Null));
        return asCell();
    }

    
    bool isObject() const { return kind() == JS::TraceKind::Object; }
    bool isScript() const { return kind() == JS::TraceKind::Script; }
    bool isString() const { return kind() == JS::TraceKind::String; }
    bool isSymbol() const { return kind() == JS::TraceKind::Symbol; }
    bool isShape() const { return kind() == JS::TraceKind::Shape; }
    bool isObjectGroup() const { return kind() == JS::TraceKind::ObjectGroup; }

    
    
    JSObject* toObject() const {
        MOZ_ASSERT(kind() == JS::TraceKind::Object);
        return reinterpret_cast<JSObject*>(asCell());
    }
    JSString* toString() const {
        MOZ_ASSERT(kind() == JS::TraceKind::String);
        return reinterpret_cast<JSString*>(asCell());
    }
    JSScript* toScript() const {
        MOZ_ASSERT(kind() == JS::TraceKind::Script);
        return reinterpret_cast<JSScript*>(asCell());
    }
    Symbol* toSymbol() const {
        MOZ_ASSERT(kind() == JS::TraceKind::Symbol);
        return reinterpret_cast<Symbol*>(asCell());
    }
    js::gc::Cell* asCell() const {
        return reinterpret_cast<js::gc::Cell*>(ptr & ~OutOfLineTraceKindMask);
    }

    
    uint64_t unsafeAsInteger() const {
        return static_cast<uint64_t>(unsafeAsUIntPtr());
    }
    
    uintptr_t unsafeAsUIntPtr() const {
        MOZ_ASSERT(asCell());
        MOZ_ASSERT(!js::gc::IsInsideNursery(asCell()));
        return reinterpret_cast<uintptr_t>(asCell());
    }

    bool mayBeOwnedByOtherRuntime() const;

  private:
    static uintptr_t checkedCast(void* p, JS::TraceKind traceKind) {
        js::gc::Cell* cell = static_cast<js::gc::Cell*>(p);
        MOZ_ASSERT((uintptr_t(p) & OutOfLineTraceKindMask) == 0);
        AssertGCThingHasType(cell, traceKind);
        
        
        MOZ_ASSERT_IF(uintptr_t(traceKind) >= OutOfLineTraceKindMask,
                      (uintptr_t(traceKind) & OutOfLineTraceKindMask) == OutOfLineTraceKindMask);
        return uintptr_t(p) | (uintptr_t(traceKind) & OutOfLineTraceKindMask);
    }

    JS::TraceKind outOfLineKind() const;

    uintptr_t ptr;
};

inline bool
operator==(const GCCellPtr& ptr1, const GCCellPtr& ptr2)
{
    return ptr1.asCell() == ptr2.asCell();
}

inline bool
operator!=(const GCCellPtr& ptr1, const GCCellPtr& ptr2)
{
    return !(ptr1 == ptr2);
}

} 

namespace js {
namespace gc {
namespace detail {

static MOZ_ALWAYS_INLINE uintptr_t*
GetGCThingMarkBitmap(const uintptr_t addr)
{
    MOZ_ASSERT(addr);
    const uintptr_t bmap_addr = (addr & ~ChunkMask) | ChunkMarkBitmapOffset;
    return reinterpret_cast<uintptr_t*>(bmap_addr);
}

static MOZ_ALWAYS_INLINE JS::shadow::Runtime*
GetGCThingRuntime(const uintptr_t addr)
{
    MOZ_ASSERT(addr);
    const uintptr_t rt_addr = (addr & ~ChunkMask) | ChunkRuntimeOffset;
    return *reinterpret_cast<JS::shadow::Runtime**>(rt_addr);
}

static MOZ_ALWAYS_INLINE void
GetGCThingMarkWordAndMask(const uintptr_t addr, uint32_t color,
                          uintptr_t** wordp, uintptr_t* maskp)
{
    MOZ_ASSERT(addr);
    const size_t bit = (addr & js::gc::ChunkMask) / js::gc::CellSize + color;
    MOZ_ASSERT(bit < js::gc::ChunkMarkBitmapBits);
    uintptr_t* bitmap = GetGCThingMarkBitmap(addr);
    const uintptr_t nbits = sizeof(*bitmap) * CHAR_BIT;
    *maskp = uintptr_t(1) << (bit % nbits);
    *wordp = &bitmap[bit / nbits];
}

static MOZ_ALWAYS_INLINE JS::Zone*
GetGCThingZone(const uintptr_t addr)
{
    MOZ_ASSERT(addr);
    const uintptr_t zone_addr = (addr & ~ArenaMask) | ArenaZoneOffset;
    return *reinterpret_cast<JS::Zone**>(zone_addr);

}

static MOZ_ALWAYS_INLINE bool
CellIsMarkedGray(const Cell* cell)
{
    MOZ_ASSERT(cell);
    MOZ_ASSERT(!js::gc::IsInsideNursery(cell));
    uintptr_t* word, mask;
    js::gc::detail::GetGCThingMarkWordAndMask(uintptr_t(cell), js::gc::GRAY, &word, &mask);
    return *word & mask;
}

} 

MOZ_ALWAYS_INLINE bool
IsInsideNursery(const js::gc::Cell* cell)
{
    if (!cell)
        return false;
    uintptr_t addr = uintptr_t(cell);
    addr &= ~js::gc::ChunkMask;
    addr |= js::gc::ChunkLocationOffset;
    uint32_t location = *reinterpret_cast<uint32_t*>(addr);
    MOZ_ASSERT(location != 0);
    return location & ChunkLocationAnyNursery;
}

} 
} 

namespace JS {

static MOZ_ALWAYS_INLINE Zone*
GetTenuredGCThingZone(GCCellPtr thing)
{
    MOZ_ASSERT(!js::gc::IsInsideNursery(thing.asCell()));
    return js::gc::detail::GetGCThingZone(thing.unsafeAsUIntPtr());
}

static MOZ_ALWAYS_INLINE Zone*
GetStringZone(JSString* str)
{
    return js::gc::detail::GetGCThingZone(uintptr_t(str));
}

extern JS_PUBLIC_API(Zone*)
GetObjectZone(JSObject* obj);

static MOZ_ALWAYS_INLINE bool
ObjectIsTenured(JSObject* obj)
{
    return !js::gc::IsInsideNursery(reinterpret_cast<js::gc::Cell*>(obj));
}

static MOZ_ALWAYS_INLINE bool
ObjectIsMarkedGray(JSObject* obj)
{
    




    if (js::gc::IsInsideNursery(reinterpret_cast<js::gc::Cell*>(obj)))
        return false;
    return js::gc::detail::CellIsMarkedGray(reinterpret_cast<js::gc::Cell*>(obj));
}

static MOZ_ALWAYS_INLINE bool
ScriptIsMarkedGray(JSScript* script)
{
    return js::gc::detail::CellIsMarkedGray(reinterpret_cast<js::gc::Cell*>(script));
}

static MOZ_ALWAYS_INLINE bool
GCThingIsMarkedGray(GCCellPtr thing)
{
    if (js::gc::IsInsideNursery(thing.asCell()))
        return false;
    if (thing.mayBeOwnedByOtherRuntime())
        return false;
    return js::gc::detail::CellIsMarkedGray(thing.asCell());
}

} 

namespace js {
namespace gc {

static MOZ_ALWAYS_INLINE bool
IsIncrementalBarrierNeededOnTenuredGCThing(JS::shadow::Runtime* rt, const JS::GCCellPtr thing)
{
    MOZ_ASSERT(thing);
    MOZ_ASSERT(!js::gc::IsInsideNursery(thing.asCell()));
    if (rt->isHeapBusy())
        return false;
    JS::Zone* zone = JS::GetTenuredGCThingZone(thing);
    return JS::shadow::Zone::asShadowZone(zone)->needsIncrementalBarrier();
}






extern JS_PUBLIC_API(JSObject*)
NewMemoryInfoObject(JSContext* cx);

} 
} 

#endif 
