





#ifndef jit_AtomicOperations_h
#define jit_AtomicOperations_h

namespace js {
namespace jit {






















class AtomicOperations
{
  public:

    
    static inline void fenceSeqCst();

    
    
    
    
    
    static inline bool isLockfree8();

    
    

    
    template<typename T>
    static inline T loadSeqCst(T* addr);

    
    template<typename T>
    static inline void storeSeqCst(T* addr, T val);

    
    template<typename T>
    static inline T exchangeSeqCst(T* addr, T val);

    
    
    template<typename T>
    static inline T compareExchangeSeqCst(T* addr, T oldval, T newval);

    
    

    
    
    template<typename T>
    static inline T fetchAddSeqCst(T* addr, T val);

    template<typename T>
    static inline T fetchSubSeqCst(T* addr, T val);

    template<typename T>
    static inline T fetchAndSeqCst(T* addr, T val);

    template<typename T>
    static inline T fetchOrSeqCst(T* addr, T val);

    template<typename T>
    static inline T fetchXorSeqCst(T* addr, T val);
};






struct RegionLock
{
  public:
    RegionLock() : spinlock(0) {}

    



    template<size_t nbytes>
    void acquire(void* addr);

    



    template<size_t nbytes>
    void release(void* addr);

  private:
    
    uint32_t spinlock;
};

} 
} 

#endif 
