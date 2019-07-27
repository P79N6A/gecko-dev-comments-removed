





#ifndef __LESWAPS_H
#define __LESWAPS_H

#include "LETypes.h"






U_NAMESPACE_BEGIN







#define SWAPW(value) LESwaps::swapWord((le_uint16)(value))







#define SWAPL(value) LESwaps::swapLong((le_uint32)(value))










class U_LAYOUT_API LESwaps  {
public:

    









    static le_uint16 swapWord(le_uint16 value)
    {
#if (defined(U_IS_BIG_ENDIAN) && U_IS_BIG_ENDIAN) || \
    (defined(BYTE_ORDER) && defined(BIG_ENDIAN) && (BYTE_ORDER == BIG_ENDIAN)) || \
    defined(__BIG_ENDIAN__)
        
        return value;
#else
        
        const le_uint8 *p = reinterpret_cast<const le_uint8 *>(&value);
        return (le_uint16)((p[0] << 8) | p[1]);
#endif
    };

    









    static le_uint32 swapLong(le_uint32 value)
    {
#if (defined(U_IS_BIG_ENDIAN) && U_IS_BIG_ENDIAN) || \
    (defined(BYTE_ORDER) && defined(BIG_ENDIAN) && (BYTE_ORDER == BIG_ENDIAN)) || \
    defined(__BIG_ENDIAN__)
        
        return value;
#else
        
        const le_uint8 *p = reinterpret_cast<const le_uint8 *>(&value);
        return (le_uint32)((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
#endif
    };

private:
    LESwaps() {} 
};

U_NAMESPACE_END
#endif
