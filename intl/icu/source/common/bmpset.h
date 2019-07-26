















#ifndef __BMPSET_H__
#define __BMPSET_H__

#include "unicode/utypes.h"
#include "unicode/uniset.h"

U_NAMESPACE_BEGIN











class BMPSet : public UMemory {
public:
    BMPSet(const int32_t *parentList, int32_t parentListLength);
    BMPSet(const BMPSet &otherBMPSet, const int32_t *newParentList, int32_t newParentListLength);
    virtual ~BMPSet();

    virtual UBool contains(UChar32 c) const;

    




    const UChar *span(const UChar *s, const UChar *limit, USetSpanCondition spanCondition) const;

    




    const UChar *spanBack(const UChar *s, const UChar *limit, USetSpanCondition spanCondition) const;

    




    const uint8_t *spanUTF8(const uint8_t *s, int32_t length, USetSpanCondition spanCondition) const;

    




    int32_t spanBackUTF8(const uint8_t *s, int32_t length, USetSpanCondition spanCondition) const;

private:
    void initBits();
    void overrideIllegal();

    
















    int32_t findCodePoint(UChar32 c, int32_t lo, int32_t hi) const;

    inline UBool containsSlow(UChar32 c, int32_t lo, int32_t hi) const;

    





    UBool asciiBytes[0xc0];

    











    uint32_t table7FF[64];

    















    uint32_t bmpBlockBits[64];

    







    int32_t list4kStarts[18];

    




    const int32_t *list;
    int32_t listLength;
};

inline UBool BMPSet::containsSlow(UChar32 c, int32_t lo, int32_t hi) const {
    return (UBool)(findCodePoint(c, lo, hi) & 1);
}

U_NAMESPACE_END

#endif
