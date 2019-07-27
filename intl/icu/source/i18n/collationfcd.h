










#ifndef __COLLATIONFCD_H__
#define __COLLATIONFCD_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/utf16.h"

U_NAMESPACE_BEGIN

































class U_I18N_API CollationFCD {
public:
    static inline UBool hasLccc(UChar32 c) {
        
        
        
        int32_t i;
        return
            
            c >= 0x300 &&
            (i = lcccIndex[c >> 5]) != 0 &&
            (lcccBits[i] & ((uint32_t)1 << (c & 0x1f))) != 0;
    }

    static inline UBool hasTccc(UChar32 c) {
        
        
        
        int32_t i;
        return
            
            c >= 0xc0 &&
            (i = tcccIndex[c >> 5]) != 0 &&
            (tcccBits[i] & ((uint32_t)1 << (c & 0x1f))) != 0;
    }

    static inline UBool mayHaveLccc(UChar32 c) {
        
        
        
        if(c < 0x300) { return FALSE; }
        if(c > 0xffff) { c = U16_LEAD(c); }
        int32_t i;
        return
            (i = lcccIndex[c >> 5]) != 0 &&
            (lcccBits[i] & ((uint32_t)1 << (c & 0x1f))) != 0;
    }

    










    static inline UBool maybeTibetanCompositeVowel(UChar32 c) {
        return (c & 0x1fff01) == 0xf01;
    }

    










    static inline UBool isFCD16OfTibetanCompositeVowel(uint16_t fcd16) {
        return fcd16 == 0x8182 || fcd16 == 0x8184;
    }

private:
    CollationFCD();  

    static const uint8_t lcccIndex[2048];
    static const uint8_t tcccIndex[2048];
    static const uint32_t lcccBits[];
    static const uint32_t tcccBits[];
};

U_NAMESPACE_END

#endif  
#endif  
