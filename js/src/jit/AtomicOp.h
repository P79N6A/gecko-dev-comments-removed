





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

inline MemoryBarrierBits
operator|(MemoryBarrierBits a, MemoryBarrierBits b)
{
    return MemoryBarrierBits(int(a) | int(b));
}

inline MemoryBarrierBits
operator&(MemoryBarrierBits a, MemoryBarrierBits b)
{
    return MemoryBarrierBits(int(a) & int(b));
}

inline MemoryBarrierBits
operator~(MemoryBarrierBits a)
{
    return MemoryBarrierBits(~int(a));
}


static const MemoryBarrierBits MembarFull = MembarLoadLoad|MembarLoadStore|MembarStoreLoad|MembarStoreStore;



static const MemoryBarrierBits MembarBeforeLoad = MembarNobits;
static const MemoryBarrierBits MembarAfterLoad = MembarLoadLoad|MembarLoadStore;
static const MemoryBarrierBits MembarBeforeStore = MembarStoreStore;
static const MemoryBarrierBits MembarAfterStore = MembarStoreLoad;

} 
} 

#endif 
