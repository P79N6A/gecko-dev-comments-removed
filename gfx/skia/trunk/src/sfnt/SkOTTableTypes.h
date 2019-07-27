






#ifndef SkOTTableTypes_DEFINED
#define SkOTTableTypes_DEFINED

#include "SkTemplates.h"
#include "SkTypes.h"
#include "SkEndian.h"


typedef uint8_t SK_OT_BYTE;
#if CHAR_BIT == 8
typedef signed char SK_OT_CHAR; 
#else
typedef int8_t SK_OT_CHAR;
#endif
typedef uint16_t SK_OT_SHORT;
typedef uint16_t SK_OT_USHORT;
typedef uint32_t SK_OT_ULONG;
typedef uint32_t SK_OT_LONG;

typedef int32_t SK_OT_Fixed;

typedef uint16_t SK_OT_F2DOT14;

typedef uint16_t SK_OT_FWORD;
typedef uint16_t SK_OT_UFWORD;

typedef uint64_t SK_OT_LONGDATETIME;

#define SK_OT_BYTE_BITFIELD SK_UINT8_BITFIELD

template<typename T> class SkOTTableTAG {
public:
    



    static const SK_OT_ULONG value = SkTEndian_SwapBE32(
        SkSetFourByteTag(T::TAG0, T::TAG1, T::TAG2, T::TAG3)
    );
};


template <unsigned N> struct SkOTSetUSHORTBit {
    SK_COMPILE_ASSERT(N < 16, NTooBig);
    static const uint16_t bit = 1u << N;
    static const SK_OT_USHORT value = SkTEndian_SwapBE16(bit);
};


template <unsigned N> struct SkOTSetULONGBit {
    SK_COMPILE_ASSERT(N < 32, NTooBig);
    static const uint32_t bit = 1u << N;
    static const SK_OT_ULONG value = SkTEndian_SwapBE32(bit);
};

#endif
