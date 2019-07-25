








#ifndef SkEndian_DEFINED
#define SkEndian_DEFINED

#include "SkTypes.h"







#if defined(SK_CPU_LENDIAN) && defined(SK_CPU_BENDIAN)
    #error "can't have both LENDIAN and BENDIAN defined"
#endif

#if !defined(SK_CPU_LENDIAN) && !defined(SK_CPU_BENDIAN)
    #error "need either LENDIAN or BENDIAN defined"
#endif




static inline uint16_t SkEndianSwap16(U16CPU value) {
    SkASSERT(value == (uint16_t)value);
    return (uint16_t)((value >> 8) | (value << 8));
}




static inline void SkEndianSwap16s(uint16_t array[], int count) {
    SkASSERT(count == 0 || array != NULL);

    while (--count >= 0) {
        *array = SkEndianSwap16(*array);
        array += 1;
    }
}




static inline uint32_t SkEndianSwap32(uint32_t value) {
    return  ((value & 0xFF) << 24) |
            ((value & 0xFF00) << 8) |
            ((value & 0xFF0000) >> 8) |
            (value >> 24);
}




static inline void SkEndianSwap32s(uint32_t array[], int count) {
    SkASSERT(count == 0 || array != NULL);

    while (--count >= 0) {
        *array = SkEndianSwap32(*array);
        array += 1;
    }
}

#ifdef SK_CPU_LENDIAN
    #define SkEndian_SwapBE16(n)    SkEndianSwap16(n)
    #define SkEndian_SwapBE32(n)    SkEndianSwap32(n)
    #define SkEndian_SwapLE16(n)    (n)
    #define SkEndian_SwapLE32(n)    (n)
#else   
    #define SkEndian_SwapBE16(n)    (n)
    #define SkEndian_SwapBE32(n)    (n)
    #define SkEndian_SwapLE16(n)    SkEndianSwap16(n)
    #define SkEndian_SwapLE32(n)    SkEndianSwap32(n)
#endif


#endif

