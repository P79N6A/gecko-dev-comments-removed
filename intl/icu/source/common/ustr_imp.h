













#ifndef __USTR_IMP_H__
#define __USTR_IMP_H__

#include "unicode/utypes.h"
#include "unicode/uiter.h"
#include "ucase.h"


#ifndef UBRK_TYPEDEF_UBREAK_ITERATOR
#   define UBRK_TYPEDEF_UBREAK_ITERATOR
    typedef struct UBreakIterator UBreakIterator;
#endif

#ifndef U_COMPARE_IGNORE_CASE





#define U_COMPARE_IGNORE_CASE       0x10000
#endif





#define _STRNCMP_STYLE 0x1000







U_CFUNC int32_t U_EXPORT2
uprv_strCompare(const UChar *s1, int32_t length1,
                const UChar *s2, int32_t length2,
                UBool strncmpStyle, UBool codePointOrder);






U_CFUNC int32_t
u_strcmpFold(const UChar *s1, int32_t length1,
             const UChar *s2, int32_t length2,
             uint32_t options,
             UErrorCode *pErrorCode);







U_CFUNC UBool
uprv_haveProperties(UErrorCode *pErrorCode);















struct UCaseMap {
    const UCaseProps *csp;
#if !UCONFIG_NO_BREAK_ITERATION
    UBreakIterator *iter;  
#endif
    char locale[32];
    int32_t locCache;
    uint32_t options;
};

#ifndef __UCASEMAP_H__
typedef struct UCaseMap UCaseMap;
#endif

#if UCONFIG_NO_BREAK_ITERATION
#   define UCASEMAP_INITIALIZER { NULL, { 0 }, 0, 0 }
#else
#   define UCASEMAP_INITIALIZER { NULL, NULL, { 0 }, 0, 0 }
#endif

U_CFUNC void
ustrcase_setTempCaseMapLocale(UCaseMap *csm, const char *locale);

#ifndef U_STRING_CASE_MAPPER_DEFINED
#define U_STRING_CASE_MAPPER_DEFINED







typedef int32_t U_CALLCONV
UStringCaseMapper(const UCaseMap *csm,
                  UChar *dest, int32_t destCapacity,
                  const UChar *src, int32_t srcLength,
                  UErrorCode *pErrorCode);

#endif


U_CFUNC int32_t U_CALLCONV
ustrcase_internalToLower(const UCaseMap *csm,
                         UChar *dest, int32_t destCapacity,
                         const UChar *src, int32_t srcLength,
                         UErrorCode *pErrorCode);


U_CFUNC int32_t U_CALLCONV
ustrcase_internalToUpper(const UCaseMap *csm,
                         UChar *dest, int32_t destCapacity,
                         const UChar *src, int32_t srcLength,
                         UErrorCode *pErrorCode);

#if !UCONFIG_NO_BREAK_ITERATION


U_CFUNC int32_t U_CALLCONV
ustrcase_internalToTitle(const UCaseMap *csm,
                         UChar *dest, int32_t destCapacity,
                         const UChar *src, int32_t srcLength,
                         UErrorCode *pErrorCode);

#endif


U_CFUNC int32_t U_CALLCONV
ustrcase_internalFold(const UCaseMap *csm,
                      UChar *dest, int32_t destCapacity,
                      const UChar *src, int32_t srcLength,
                      UErrorCode *pErrorCode);





U_CFUNC int32_t
ustrcase_map(const UCaseMap *csm,
             UChar *dest, int32_t destCapacity,
             const UChar *src, int32_t srcLength,
             UStringCaseMapper *stringCaseMapper,
             UErrorCode *pErrorCode);








typedef int32_t U_CALLCONV
UTF8CaseMapper(const UCaseMap *csm,
               uint8_t *dest, int32_t destCapacity,
               const uint8_t *src, int32_t srcLength,
               UErrorCode *pErrorCode);


U_CFUNC int32_t U_CALLCONV
ucasemap_internalUTF8ToTitle(const UCaseMap *csm,
         uint8_t *dest, int32_t destCapacity,
         const uint8_t *src, int32_t srcLength,
         UErrorCode *pErrorCode);





U_CFUNC int32_t
ucasemap_mapUTF8(const UCaseMap *csm,
                 uint8_t *dest, int32_t destCapacity,
                 const uint8_t *src, int32_t srcLength,
                 UTF8CaseMapper *stringCaseMapper,
                 UErrorCode *pErrorCode);

U_CAPI int32_t U_EXPORT2 
ustr_hashUCharsN(const UChar *str, int32_t length);

U_CAPI int32_t U_EXPORT2 
ustr_hashCharsN(const char *str, int32_t length);

U_CAPI int32_t U_EXPORT2
ustr_hashICharsN(const char *str, int32_t length);













U_CAPI int32_t U_EXPORT2
u_terminateUChars(UChar *dest, int32_t destCapacity, int32_t length, UErrorCode *pErrorCode);





U_CAPI int32_t U_EXPORT2
u_terminateChars(char *dest, int32_t destCapacity, int32_t length, UErrorCode *pErrorCode);





U_CAPI int32_t U_EXPORT2
u_terminateUChar32s(UChar32 *dest, int32_t destCapacity, int32_t length, UErrorCode *pErrorCode);





U_CAPI int32_t U_EXPORT2
u_terminateWChars(wchar_t *dest, int32_t destCapacity, int32_t length, UErrorCode *pErrorCode);

#endif
