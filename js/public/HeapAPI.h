





#ifndef js_heap_api_h___
#define js_heap_api_h___


namespace js {
namespace gc {







#if (defined(SOLARIS) || defined(__FreeBSD__)) && \
    (defined(__sparc) || defined(__sparcv9) || defined(__ia64))
const size_t PageShift = 13;
const size_t ArenaShift = PageShift;
#elif defined(__powerpc__)
const size_t PageShift = 16;
const size_t ArenaShift = 12;
#else
const size_t PageShift = 12;
const size_t ArenaShift = PageShift;
#endif
const size_t PageSize = size_t(1) << PageShift;
const size_t ArenaSize = size_t(1) << ArenaShift;
const size_t ArenaMask = ArenaSize - 1;

const size_t ChunkShift = 20;
const size_t ChunkSize = size_t(1) << ChunkShift;
const size_t ChunkMask = ChunkSize - 1;

} 
} 

namespace JS {

namespace shadow {

struct ArenaHeader
{
    JSCompartment *compartment;
};

} 

static inline shadow::ArenaHeader *
GetGCThingArena(void *thing)
{
    uintptr_t addr = uintptr_t(thing);
    addr &= ~js::gc::ArenaMask;
    return reinterpret_cast<shadow::ArenaHeader *>(addr);
}

static inline JSCompartment *
GetGCThingCompartment(void *thing)
{
    JS_ASSERT(thing);
    return GetGCThingArena(thing)->compartment;
}

static inline JSCompartment *
GetObjectCompartment(JSObject *obj)
{
    return GetGCThingCompartment(obj);
}

} 

#endif 
