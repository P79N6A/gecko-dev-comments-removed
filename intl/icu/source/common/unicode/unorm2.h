















#ifndef __UNORM2_H__
#define __UNORM2_H__












#include "unicode/utypes.h"
#include "unicode/localpointer.h"
#include "unicode/uset.h"








typedef enum {
    







    UNORM2_COMPOSE,
    







    UNORM2_DECOMPOSE,
    











    UNORM2_FCD,
    








    UNORM2_COMPOSE_CONTIGUOUS
} UNormalization2Mode;






typedef enum UNormalizationCheckResult {
  



  UNORM_NO,
  



  UNORM_YES,
  






  UNORM_MAYBE
} UNormalizationCheckResult;





struct UNormalizer2;
typedef struct UNormalizer2 UNormalizer2;  

#if !UCONFIG_NO_NORMALIZATION

#ifndef U_HIDE_DRAFT_API











U_DRAFT const UNormalizer2 * U_EXPORT2
unorm2_getNFCInstance(UErrorCode *pErrorCode);












U_DRAFT const UNormalizer2 * U_EXPORT2
unorm2_getNFDInstance(UErrorCode *pErrorCode);












U_DRAFT const UNormalizer2 * U_EXPORT2
unorm2_getNFKCInstance(UErrorCode *pErrorCode);












U_DRAFT const UNormalizer2 * U_EXPORT2
unorm2_getNFKDInstance(UErrorCode *pErrorCode);












U_DRAFT const UNormalizer2 * U_EXPORT2
unorm2_getNFKCCasefoldInstance(UErrorCode *pErrorCode);
#endif  






















U_STABLE const UNormalizer2 * U_EXPORT2
unorm2_getInstance(const char *packageName,
                   const char *name,
                   UNormalization2Mode mode,
                   UErrorCode *pErrorCode);
















U_STABLE UNormalizer2 * U_EXPORT2
unorm2_openFiltered(const UNormalizer2 *norm2, const USet *filterSet, UErrorCode *pErrorCode);







U_STABLE void U_EXPORT2
unorm2_close(UNormalizer2 *norm2);

#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN










U_DEFINE_LOCAL_OPEN_POINTER(LocalUNormalizer2Pointer, UNormalizer2, unorm2_close);

U_NAMESPACE_END

#endif

















U_STABLE int32_t U_EXPORT2
unorm2_normalize(const UNormalizer2 *norm2,
                 const UChar *src, int32_t length,
                 UChar *dest, int32_t capacity,
                 UErrorCode *pErrorCode);


















U_STABLE int32_t U_EXPORT2
unorm2_normalizeSecondAndAppend(const UNormalizer2 *norm2,
                                UChar *first, int32_t firstLength, int32_t firstCapacity,
                                const UChar *second, int32_t secondLength,
                                UErrorCode *pErrorCode);


















U_STABLE int32_t U_EXPORT2
unorm2_append(const UNormalizer2 *norm2,
              UChar *first, int32_t firstLength, int32_t firstCapacity,
              const UChar *second, int32_t secondLength,
              UErrorCode *pErrorCode);




















U_STABLE int32_t U_EXPORT2
unorm2_getDecomposition(const UNormalizer2 *norm2,
                        UChar32 c, UChar *decomposition, int32_t capacity,
                        UErrorCode *pErrorCode);

#ifndef U_HIDE_DRAFT_API





























U_DRAFT int32_t U_EXPORT2
unorm2_getRawDecomposition(const UNormalizer2 *norm2,
                           UChar32 c, UChar *decomposition, int32_t capacity,
                           UErrorCode *pErrorCode);
















U_DRAFT UChar32 U_EXPORT2
unorm2_composePair(const UNormalizer2 *norm2, UChar32 a, UChar32 b);










U_DRAFT uint8_t U_EXPORT2
unorm2_getCombiningClass(const UNormalizer2 *norm2, UChar32 c);
#endif  

















U_STABLE UBool U_EXPORT2
unorm2_isNormalized(const UNormalizer2 *norm2,
                    const UChar *s, int32_t length,
                    UErrorCode *pErrorCode);


















U_STABLE UNormalizationCheckResult U_EXPORT2
unorm2_quickCheck(const UNormalizer2 *norm2,
                  const UChar *s, int32_t length,
                  UErrorCode *pErrorCode);

























U_STABLE int32_t U_EXPORT2
unorm2_spanQuickCheckYes(const UNormalizer2 *norm2,
                         const UChar *s, int32_t length,
                         UErrorCode *pErrorCode);










U_STABLE UBool U_EXPORT2
unorm2_hasBoundaryBefore(const UNormalizer2 *norm2, UChar32 c);










U_STABLE UBool U_EXPORT2
unorm2_hasBoundaryAfter(const UNormalizer2 *norm2, UChar32 c);









U_STABLE UBool U_EXPORT2
unorm2_isInert(const UNormalizer2 *norm2, UChar32 c);

#endif  
#endif  
