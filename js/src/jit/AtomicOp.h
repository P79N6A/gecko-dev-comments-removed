





#ifndef jit_AtomicOp_h
#define jit_AtomicOp_h

namespace js {
namespace jit {



enum AtomicOp {
    AtomicFetchAddOp,
    AtomicFetchSubOp,
    AtomicFetchAndOp,
    AtomicFetchOrOp,
    AtomicFetchXorOp
};







enum MemoryBarrierBits {
    MembarLoadLoad = 1,
    MembarLoadStore = 2,
    MembarStoreStore = 4,
    MembarStoreLoad = 8,

    MembarSynchronizing = 16,

    
    MembarNobits = 0,
    MembarAllbits = 31,
};

static inline MOZ_CONSTEXPR MemoryBarrierBits
operator|(MemoryBarrierBits a, MemoryBarrierBits b)
{
    return MemoryBarrierBits(int(a) | int(b));
}

static inline MOZ_CONSTEXPR MemoryBarrierBits
operator&(MemoryBarrierBits a, MemoryBarrierBits b)
{
    return MemoryBarrierBits(int(a) & int(b));
}

static inline MOZ_CONSTEXPR MemoryBarrierBits
operator~(MemoryBarrierBits a)
{
    return MemoryBarrierBits(~int(a));
}


static MOZ_CONSTEXPR_VAR MemoryBarrierBits MembarFull = MembarLoadLoad|MembarLoadStore|MembarStoreLoad|MembarStoreStore;



static MOZ_CONSTEXPR_VAR MemoryBarrierBits MembarBeforeLoad = MembarNobits;
static MOZ_CONSTEXPR_VAR MemoryBarrierBits MembarAfterLoad = MembarLoadLoad|MembarLoadStore;
static MOZ_CONSTEXPR_VAR MemoryBarrierBits MembarBeforeStore = MembarStoreStore;
static MOZ_CONSTEXPR_VAR MemoryBarrierBits MembarAfterStore = MembarStoreLoad;

} 
} 

#endif 
