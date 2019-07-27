















#ifndef __COLLATIONWEIGHTS_H__
#define __COLLATIONWEIGHTS_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/uobject.h"

U_NAMESPACE_BEGIN





class U_I18N_API CollationWeights : public UMemory {
public:
    CollationWeights();

    static inline int32_t lengthOfWeight(uint32_t weight) {
        if((weight&0xffffff)==0) {
            return 1;
        } else if((weight&0xffff)==0) {
            return 2;
        } else if((weight&0xff)==0) {
            return 3;
        } else {
            return 4;
        }
    }

    void initForPrimary(UBool compressible);
    void initForSecondary();
    void initForTertiary();

    












    UBool allocWeights(uint32_t lowerLimit, uint32_t upperLimit, int32_t n);

    






    uint32_t nextWeight();

    
    struct WeightRange {
        uint32_t start, end;
        int32_t length, count;
    };

private:
    
    inline int32_t countBytes(int32_t idx) const {
        return (int32_t)(maxBytes[idx] - minBytes[idx] + 1);
    }

    uint32_t incWeight(uint32_t weight, int32_t length) const;
    uint32_t incWeightByOffset(uint32_t weight, int32_t length, int32_t offset) const;
    void lengthenRange(WeightRange &range) const;
    




    UBool getWeightRanges(uint32_t lowerLimit, uint32_t upperLimit);
    UBool allocWeightsInShortRanges(int32_t n, int32_t minLength);
    UBool allocWeightsInMinLengthRanges(int32_t n, int32_t minLength);

    int32_t middleLength;
    uint32_t minBytes[5];  
    uint32_t maxBytes[5];
    WeightRange ranges[7];
    int32_t rangeIndex;
    int32_t rangeCount;
};

U_NAMESPACE_END

#endif  
#endif  
