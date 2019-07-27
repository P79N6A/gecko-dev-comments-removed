










#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "collation.h"
#include "uassert.h"

U_NAMESPACE_BEGIN



#ifndef _MSC_VER
const uint8_t Collation::LEVEL_SEPARATOR_BYTE;
const uint8_t Collation::MERGE_SEPARATOR_BYTE;
const uint32_t Collation::ONLY_TERTIARY_MASK;
const uint32_t Collation::CASE_AND_TERTIARY_MASK;
#endif

uint32_t
Collation::incTwoBytePrimaryByOffset(uint32_t basePrimary, UBool isCompressible, int32_t offset) {
    
    
    
    uint32_t primary;
    if(isCompressible) {
        offset += ((int32_t)(basePrimary >> 16) & 0xff) - 4;
        primary = (uint32_t)((offset % 251) + 4) << 16;
        offset /= 251;
    } else {
        offset += ((int32_t)(basePrimary >> 16) & 0xff) - 2;
        primary = (uint32_t)((offset % 254) + 2) << 16;
        offset /= 254;
    }
    
    return primary | ((basePrimary & 0xff000000) + (uint32_t)(offset << 24));
}

uint32_t
Collation::incThreeBytePrimaryByOffset(uint32_t basePrimary, UBool isCompressible, int32_t offset) {
    
    
    offset += ((int32_t)(basePrimary >> 8) & 0xff) - 2;
    uint32_t primary = (uint32_t)((offset % 254) + 2) << 8;
    offset /= 254;
    
    
    if(isCompressible) {
        offset += ((int32_t)(basePrimary >> 16) & 0xff) - 4;
        primary |= (uint32_t)((offset % 251) + 4) << 16;
        offset /= 251;
    } else {
        offset += ((int32_t)(basePrimary >> 16) & 0xff) - 2;
        primary |= (uint32_t)((offset % 254) + 2) << 16;
        offset /= 254;
    }
    
    return primary | ((basePrimary & 0xff000000) + (uint32_t)(offset << 24));
}

uint32_t
Collation::decTwoBytePrimaryByOneStep(uint32_t basePrimary, UBool isCompressible, int32_t step) {
    
    
    
    
    U_ASSERT(0 < step && step <= 0x7f);
    int32_t byte2 = ((int32_t)(basePrimary >> 16) & 0xff) - step;
    if(isCompressible) {
        if(byte2 < 4) {
            byte2 += 251;
            basePrimary -= 0x1000000;
        }
    } else {
        if(byte2 < 2) {
            byte2 += 254;
            basePrimary -= 0x1000000;
        }
    }
    return (basePrimary & 0xff000000) | ((uint32_t)byte2 << 16);
}

uint32_t
Collation::decThreeBytePrimaryByOneStep(uint32_t basePrimary, UBool isCompressible, int32_t step) {
    
    
    U_ASSERT(0 < step && step <= 0x7f);
    int32_t byte3 = ((int32_t)(basePrimary >> 8) & 0xff) - step;
    if(byte3 >= 2) {
        return (basePrimary & 0xffff0000) | ((uint32_t)byte3 << 8);
    }
    byte3 += 254;
    
    
    int32_t byte2 = ((int32_t)(basePrimary >> 16) & 0xff) - 1;
    if(isCompressible) {
        if(byte2 < 4) {
            byte2 = 0xfe;
            basePrimary -= 0x1000000;
        }
    } else {
        if(byte2 < 2) {
            byte2 = 0xff;
            basePrimary -= 0x1000000;
        }
    }
    
    return (basePrimary & 0xff000000) | ((uint32_t)byte2 << 16) | ((uint32_t)byte3 << 8);
}

uint32_t
Collation::getThreeBytePrimaryForOffsetData(UChar32 c, int64_t dataCE) {
    uint32_t p = (uint32_t)(dataCE >> 32);  
    int32_t lower32 = (int32_t)dataCE;  
    int32_t offset = (c - (lower32 >> 8)) * (lower32 & 0x7f);  
    UBool isCompressible = (lower32 & 0x80) != 0;
    return Collation::incThreeBytePrimaryByOffset(p, isCompressible, offset);
}

uint32_t
Collation::unassignedPrimaryFromCodePoint(UChar32 c) {
    
    ++c;
    
    uint32_t primary = 2 + (c % 18) * 14;
    c /= 18;
    
    primary |= (2 + (c % 254)) << 8;
    c /= 254;
    
    primary |= (4 + (c % 251)) << 16;
    
    return primary | (UNASSIGNED_IMPLICIT_BYTE << 24);
}

U_NAMESPACE_END

#endif  
