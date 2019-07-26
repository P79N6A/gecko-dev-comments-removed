






#ifndef SkOTTableTypes_DEFINED
#define SkOTTableTypes_DEFINED

#include "SkTypes.h"
#include "SkEndian.h"


typedef uint8_t SK_OT_BYTE;
#if CHAR_BIT == 8
typedef signed char SK_OT_CHAR; 
#else
typedef int8_t SK_OT_CHAR;
#endif
typedef int16_t SK_OT_SHORT;
typedef uint16_t SK_OT_USHORT;
typedef uint32_t SK_OT_ULONG;
typedef int32_t SK_OT_LONG;

typedef int32_t SK_OT_Fixed;

typedef int16_t SK_OT_FWORD;
typedef uint16_t SK_OT_UFWORD;

typedef uint64_t SK_OT_LONGDATETIME;

#define SK_OT_BYTE_BITFIELD SK_UINT8_BITFIELD

template<typename T> class SkOTTableTAG {
public:
    



    static const SK_OT_ULONG value = SkTEndian_SwapBE32(
        SkSetFourByteTag(T::TAG0, T::TAG1, T::TAG2, T::TAG3)
    );
};

#endif
