















#ifndef __UIDNA_H__
#define __UIDNA_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_IDNA

#include "unicode/localpointer.h"
#include "unicode/parseerr.h"
















enum {
    




    UIDNA_DEFAULT=0,
    






    UIDNA_ALLOW_UNASSIGNED=1,
    






    UIDNA_USE_STD3_RULES=2,
    






    UIDNA_CHECK_BIDI=4,
    






    UIDNA_CHECK_CONTEXTJ=8,
    







    UIDNA_NONTRANSITIONAL_TO_ASCII=0x10,
    







    UIDNA_NONTRANSITIONAL_TO_UNICODE=0x20,
#ifndef U_HIDE_DRAFT_API
    








    UIDNA_CHECK_CONTEXTO=0x40
#endif  
};





struct UIDNA;
typedef struct UIDNA UIDNA;  


















U_STABLE UIDNA * U_EXPORT2
uidna_openUTS46(uint32_t options, UErrorCode *pErrorCode);






U_STABLE void U_EXPORT2
uidna_close(UIDNA *idna);

#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN










U_DEFINE_LOCAL_OPEN_POINTER(LocalUIDNAPointer, UIDNA, uidna_close);

U_NAMESPACE_END

#endif











typedef struct UIDNAInfo {
    
    int16_t size;
    




    UBool isTransitionalDifferent;
    UBool reservedB3;  
    




    uint32_t errors;
    int32_t reservedI2;  
    int32_t reservedI3;  
} UIDNAInfo;





#define UIDNA_INFO_INITIALIZER { \
    (int16_t)sizeof(UIDNAInfo), \
    FALSE, FALSE, \
    0, 0, 0 }
























U_STABLE int32_t U_EXPORT2
uidna_labelToASCII(const UIDNA *idna,
                   const UChar *label, int32_t length,
                   UChar *dest, int32_t capacity,
                   UIDNAInfo *pInfo, UErrorCode *pErrorCode);






















U_STABLE int32_t U_EXPORT2
uidna_labelToUnicode(const UIDNA *idna,
                     const UChar *label, int32_t length,
                     UChar *dest, int32_t capacity,
                     UIDNAInfo *pInfo, UErrorCode *pErrorCode);
























U_STABLE int32_t U_EXPORT2
uidna_nameToASCII(const UIDNA *idna,
                  const UChar *name, int32_t length,
                  UChar *dest, int32_t capacity,
                  UIDNAInfo *pInfo, UErrorCode *pErrorCode);






















U_STABLE int32_t U_EXPORT2
uidna_nameToUnicode(const UIDNA *idna,
                    const UChar *name, int32_t length,
                    UChar *dest, int32_t capacity,
                    UIDNAInfo *pInfo, UErrorCode *pErrorCode);




















U_STABLE int32_t U_EXPORT2
uidna_labelToASCII_UTF8(const UIDNA *idna,
                        const char *label, int32_t length,
                        char *dest, int32_t capacity,
                        UIDNAInfo *pInfo, UErrorCode *pErrorCode);


















U_STABLE int32_t U_EXPORT2
uidna_labelToUnicodeUTF8(const UIDNA *idna,
                         const char *label, int32_t length,
                         char *dest, int32_t capacity,
                         UIDNAInfo *pInfo, UErrorCode *pErrorCode);


















U_STABLE int32_t U_EXPORT2
uidna_nameToASCII_UTF8(const UIDNA *idna,
                       const char *name, int32_t length,
                       char *dest, int32_t capacity,
                       UIDNAInfo *pInfo, UErrorCode *pErrorCode);


















U_STABLE int32_t U_EXPORT2
uidna_nameToUnicodeUTF8(const UIDNA *idna,
                        const char *name, int32_t length,
                        char *dest, int32_t capacity,
                        UIDNAInfo *pInfo, UErrorCode *pErrorCode);






enum {
    



    UIDNA_ERROR_EMPTY_LABEL=1,
    





    UIDNA_ERROR_LABEL_TOO_LONG=2,
    





    UIDNA_ERROR_DOMAIN_NAME_TOO_LONG=4,
    



    UIDNA_ERROR_LEADING_HYPHEN=8,
    



    UIDNA_ERROR_TRAILING_HYPHEN=0x10,
    



    UIDNA_ERROR_HYPHEN_3_4=0x20,
    



    UIDNA_ERROR_LEADING_COMBINING_MARK=0x40,
    



    UIDNA_ERROR_DISALLOWED=0x80,
    




    UIDNA_ERROR_PUNYCODE=0x100,
    




    UIDNA_ERROR_LABEL_HAS_DOT=0x200,
    







    UIDNA_ERROR_INVALID_ACE_LABEL=0x400,
    



    UIDNA_ERROR_BIDI=0x800,
    



    UIDNA_ERROR_CONTEXTJ=0x1000,
#ifndef U_HIDE_DRAFT_API
    





    UIDNA_ERROR_CONTEXTO_PUNCTUATION=0x2000,
    




    UIDNA_ERROR_CONTEXTO_DIGITS=0x4000
#endif  
};






























































U_STABLE int32_t U_EXPORT2
uidna_toASCII(const UChar* src, int32_t srcLength, 
              UChar* dest, int32_t destCapacity,
              int32_t options,
              UParseError* parseError,
              UErrorCode* status);










































U_STABLE int32_t U_EXPORT2
uidna_toUnicode(const UChar* src, int32_t srcLength,
                UChar* dest, int32_t destCapacity,
                int32_t options,
                UParseError* parseError,
                UErrorCode* status);













































U_STABLE int32_t U_EXPORT2
uidna_IDNToASCII(  const UChar* src, int32_t srcLength,
                   UChar* dest, int32_t destCapacity,
                   int32_t options,
                   UParseError* parseError,
                   UErrorCode* status);









































U_STABLE int32_t U_EXPORT2
uidna_IDNToUnicode(  const UChar* src, int32_t srcLength,
                     UChar* dest, int32_t destCapacity,
                     int32_t options,
                     UParseError* parseError,
                     UErrorCode* status);



































U_STABLE int32_t U_EXPORT2
uidna_compare(  const UChar *s1, int32_t length1,
                const UChar *s2, int32_t length2,
                int32_t options,
                UErrorCode* status);

#endif 

#endif
