










#ifndef __COLLATIONDATAWRITER_H__
#define __COLLATIONDATAWRITER_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

U_NAMESPACE_BEGIN

struct CollationData;
struct CollationSettings;
struct CollationTailoring;




class U_I18N_API CollationDataWriter  {
public:
    static int32_t writeBase(const CollationData &data, const CollationSettings &settings,
                             const void *rootElements, int32_t rootElementsLength,
                             int32_t indexes[], uint8_t *dest, int32_t capacity,
                             UErrorCode &errorCode);

    static int32_t writeTailoring(const CollationTailoring &t, const CollationSettings &settings,
                                  int32_t indexes[], uint8_t *dest, int32_t capacity,
                                  UErrorCode &errorCode);

private:
    CollationDataWriter();  

    static int32_t write(UBool isBase, const UVersionInfo dataVersion,
                         const CollationData &data, const CollationSettings &settings,
                         const void *rootElements, int32_t rootElementsLength,
                         int32_t indexes[], uint8_t *dest, int32_t capacity,
                         UErrorCode &errorCode);

    static void copyData(const int32_t indexes[], int32_t startIndex,
                         const void *src, uint8_t *dest);
};

U_NAMESPACE_END

#endif  
#endif  
