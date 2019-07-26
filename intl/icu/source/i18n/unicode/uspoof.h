















#ifndef USPOOF_H
#define USPOOF_H

#include "unicode/utypes.h"
#include "unicode/uset.h"
#include "unicode/parseerr.h"
#include "unicode/localpointer.h"

#if !UCONFIG_NO_NORMALIZATION


#if U_SHOW_CPLUSPLUS_API
#include "unicode/unistr.h"
#include "unicode/uniset.h"
#endif
















































































































struct USpoofChecker;
typedef struct USpoofChecker USpoofChecker; 








typedef enum USpoofChecks {
    




    USPOOF_SINGLE_SCRIPT_CONFUSABLE =   1,

    







    USPOOF_MIXED_SCRIPT_CONFUSABLE  =   2,

    








    USPOOF_WHOLE_SCRIPT_CONFUSABLE  =   4,
    
    





    USPOOF_ANY_CASE                 =   8,

    



    USPOOF_SINGLE_SCRIPT            =  16,
    
    





    USPOOF_INVISIBLE                =  32,

    



    USPOOF_CHAR_LIMIT               =  64,

    USPOOF_ALL_CHECKS               = 0x7f
    } USpoofChecks;
    
    










U_STABLE USpoofChecker * U_EXPORT2
uspoof_open(UErrorCode *status);























U_STABLE USpoofChecker * U_EXPORT2
uspoof_openFromSerialized(const void *data, int32_t length, int32_t *pActualLength,
                          UErrorCode *pErrorCode);
































U_STABLE USpoofChecker * U_EXPORT2
uspoof_openFromSource(const char *confusables,  int32_t confusablesLen,
                      const char *confusablesWholeScript, int32_t confusablesWholeScriptLen,
                      int32_t *errType, UParseError *pe, UErrorCode *status);







U_STABLE void U_EXPORT2
uspoof_close(USpoofChecker *sc);

#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN










U_DEFINE_LOCAL_OPEN_POINTER(LocalUSpoofCheckerPointer, USpoofChecker, uspoof_close);

U_NAMESPACE_END

#endif










U_STABLE USpoofChecker * U_EXPORT2
uspoof_clone(const USpoofChecker *sc, UErrorCode *status);














U_STABLE void U_EXPORT2
uspoof_setChecks(USpoofChecker *sc, int32_t checks, UErrorCode *status);












U_STABLE int32_t U_EXPORT2
uspoof_getChecks(const USpoofChecker *sc, UErrorCode *status);











































U_STABLE void U_EXPORT2
uspoof_setAllowedLocales(USpoofChecker *sc, const char *localesList, UErrorCode *status);






















U_STABLE const char * U_EXPORT2
uspoof_getAllowedLocales(USpoofChecker *sc, UErrorCode *status);




















U_STABLE void U_EXPORT2
uspoof_setAllowedChars(USpoofChecker *sc, const USet *chars, UErrorCode *status);






















U_STABLE const USet * U_EXPORT2
uspoof_getAllowedChars(const USpoofChecker *sc, UErrorCode *status);


#if U_SHOW_CPLUSPLUS_API


















U_STABLE void U_EXPORT2
uspoof_setAllowedUnicodeSet(USpoofChecker *sc, const icu::UnicodeSet *chars, UErrorCode *status);






















U_STABLE const icu::UnicodeSet * U_EXPORT2
uspoof_getAllowedUnicodeSet(const USpoofChecker *sc, UErrorCode *status);
#endif






























U_STABLE int32_t U_EXPORT2
uspoof_check(const USpoofChecker *sc,
                         const UChar *text, int32_t length, 
                         int32_t *position,
                         UErrorCode *status);






























U_STABLE int32_t U_EXPORT2
uspoof_checkUTF8(const USpoofChecker *sc,
                 const char *text, int32_t length,
                 int32_t *position,
                 UErrorCode *status);


#if U_SHOW_CPLUSPLUS_API

























U_STABLE int32_t U_EXPORT2
uspoof_checkUnicodeString(const USpoofChecker *sc,
                          const icu::UnicodeString &text, 
                          int32_t *position,
                          UErrorCode *status);

#endif









































U_STABLE int32_t U_EXPORT2
uspoof_areConfusable(const USpoofChecker *sc,
                     const UChar *s1, int32_t length1,
                     const UChar *s2, int32_t length2,
                     UErrorCode *status);




























U_STABLE int32_t U_EXPORT2
uspoof_areConfusableUTF8(const USpoofChecker *sc,
                         const char *s1, int32_t length1,
                         const char *s2, int32_t length2,
                         UErrorCode *status);




#if U_SHOW_CPLUSPLUS_API





















U_STABLE int32_t U_EXPORT2
uspoof_areConfusableUnicodeString(const USpoofChecker *sc,
                                  const icu::UnicodeString &s1,
                                  const icu::UnicodeString &s2,
                                  UErrorCode *status);
#endif


































U_STABLE int32_t U_EXPORT2
uspoof_getSkeleton(const USpoofChecker *sc,
                   uint32_t type,
                   const UChar *s,  int32_t length,
                   UChar *dest, int32_t destCapacity,
                   UErrorCode *status);
    


































   
U_STABLE int32_t U_EXPORT2
uspoof_getSkeletonUTF8(const USpoofChecker *sc,
                       uint32_t type,
                       const char *s,  int32_t length,
                       char *dest, int32_t destCapacity,
                       UErrorCode *status);
    
#if U_SHOW_CPLUSPLUS_API
























   
U_I18N_API icu::UnicodeString & U_EXPORT2
uspoof_getSkeletonUnicodeString(const USpoofChecker *sc,
                                uint32_t type,
                                const icu::UnicodeString &s,
                                icu::UnicodeString &dest,
                                UErrorCode *status);
#endif   




















U_STABLE int32_t U_EXPORT2
uspoof_serialize(USpoofChecker *sc,
                 void *data, int32_t capacity,
                 UErrorCode *status);


#endif

#endif   
