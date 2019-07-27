





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

    
    MembarAllbits = 31,
};


static const int MembarFull = MembarLoadLoad|MembarLoadStore|MembarStoreLoad|MembarStoreStore;



static const int MembarBeforeLoad = 0;
static const int MembarAfterLoad = MembarLoadLoad|MembarLoadStore;
static const int MembarBeforeStore = MembarStoreStore;
static const int MembarAfterStore = MembarStoreLoad;

} 
} 

#endif 
