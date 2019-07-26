


























#ifndef yarr_YarrCanonicalizeUCS2_h
#define yarr_YarrCanonicalizeUCS2_h

#include <stdint.h>

#include "yarr/wtfbridge.h"

namespace JSC { namespace Yarr {




enum UCS2CanonicalizationType {
    CanonicalizeUnique,               
    CanonicalizeSet,                  
    CanonicalizeRangeLo,              
    CanonicalizeRangeHi,              
    CanonicalizeAlternatingAligned,   
    CanonicalizeAlternatingUnaligned  
};
struct UCS2CanonicalizationRange { uint16_t begin, end, value, type; };
extern const size_t UCS2_CANONICALIZATION_RANGES;
extern const uint16_t* const characterSetInfo[];
extern const UCS2CanonicalizationRange rangeInfo[];



enum LatinCanonicalizationType {
    CanonicalizeLatinSelf,     
    CanonicalizeLatinMask0x20, 
    CanonicalizeLatinOther,    
    CanonicalizeLatinInvalid   
};
struct LatinCanonicalizationRange { uint16_t begin, end, value, type; };
extern const size_t LATIN_CANONICALIZATION_RANGES;
extern const LatinCanonicalizationRange latinRangeInfo[];


inline const UCS2CanonicalizationRange* rangeInfoFor(UChar ch)
{
    const UCS2CanonicalizationRange* info = rangeInfo;
    size_t entries = UCS2_CANONICALIZATION_RANGES;

    while (true) {
        size_t candidate = entries >> 1;
        const UCS2CanonicalizationRange* candidateInfo = info + candidate;
        if (ch < candidateInfo->begin)
            entries = candidate;
        else if (ch <= candidateInfo->end)
            return candidateInfo;
        else {
            info = candidateInfo + 1;
            entries -= (candidate + 1);
        }
    }
}


inline UChar getCanonicalPair(const UCS2CanonicalizationRange* info, UChar ch)
{
    ASSERT(ch >= info->begin && ch <= info->end);
    switch (info->type) {
    case CanonicalizeRangeLo:
        return ch + info->value;
    case CanonicalizeRangeHi:
        return ch - info->value;
    case CanonicalizeAlternatingAligned:
        return ch ^ 1;
    case CanonicalizeAlternatingUnaligned:
        return ((ch - 1) ^ 1) + 1;
    default:
        ASSERT_NOT_REACHED();
    }
    ASSERT_NOT_REACHED();
    return 0;
}


inline bool isCanonicallyUnique(UChar ch)
{
    return rangeInfoFor(ch)->type == CanonicalizeUnique;
}


inline bool areCanonicallyEquivalent(UChar a, UChar b)
{
    const UCS2CanonicalizationRange* info = rangeInfoFor(a);
    switch (info->type) {
    case CanonicalizeUnique:
        return a == b;
    case CanonicalizeSet: {
        for (const uint16_t* set = characterSetInfo[info->value]; (a = *set); ++set) {
            if (a == b)
                return true;
        }
        return false;
    }
    case CanonicalizeRangeLo:
        return (a == b) || (a + info->value == b);
    case CanonicalizeRangeHi:
        return (a == b) || (a - info->value == b);
    case CanonicalizeAlternatingAligned:
        return (a | 1) == (b | 1);
    case CanonicalizeAlternatingUnaligned:
        return ((a - 1) | 1) == ((b - 1) | 1);
    }

    ASSERT_NOT_REACHED();
    return false;
}

} } 

#endif 
