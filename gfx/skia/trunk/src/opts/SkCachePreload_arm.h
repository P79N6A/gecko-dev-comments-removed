





#ifndef SkCachePreload_arm_DEFINED
#define SkCachePreload_arm_DEFINED






#if defined(__ARM_USE_PLD)

#define PLD(x, n)           "pld        [%["#x"], #("#n")]\n\t"

#if __ARM_CACHE_LINE_SIZE == 32
    #define PLD64(x, n)      PLD(x, n) PLD(x, (n) + 32)
#elif __ARM_CACHE_LINE_SIZE == 64
    #define PLD64(x, n)      PLD(x, n)
#else
    #error "unknown __ARM_CACHE_LINE_SIZE."
#endif
#else
    
    #define PLD(x, n)
    #define PLD64(x, n)
#endif

#define PLD128(x, n)         PLD64(x, n) PLD64(x, (n) + 64)

#endif  
