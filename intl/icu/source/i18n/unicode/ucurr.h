





#ifndef _UCURR_H_
#define _UCURR_H_

#include "unicode/utypes.h"
#include "unicode/uenum.h"






#if !UCONFIG_NO_FORMATTING

































U_STABLE int32_t U_EXPORT2
ucurr_forLocale(const char* locale,
                UChar* buff,
                int32_t buffCapacity,
                UErrorCode* ec);







typedef enum UCurrNameStyle {
    




    UCURR_SYMBOL_NAME,

    




    UCURR_LONG_NAME
} UCurrNameStyle;

#if !UCONFIG_NO_SERVICE



typedef const void* UCurrRegistryKey;












U_STABLE UCurrRegistryKey U_EXPORT2
ucurr_register(const UChar* isoCode, 
                   const char* locale,  
                   UErrorCode* status);











U_STABLE UBool U_EXPORT2
ucurr_unregister(UCurrRegistryKey key, UErrorCode* status);
#endif 


















U_STABLE const UChar* U_EXPORT2
ucurr_getName(const UChar* currency,
              const char* locale,
              UCurrNameStyle nameStyle,
              UBool* isChoiceFormat,
              int32_t* len,
              UErrorCode* ec);

















U_STABLE const UChar* U_EXPORT2
ucurr_getPluralName(const UChar* currency,
                    const char* locale,
                    UBool* isChoiceFormat,
                    const char* pluralCount,
                    int32_t* len,
                    UErrorCode* ec);










U_STABLE int32_t U_EXPORT2
ucurr_getDefaultFractionDigits(const UChar* currency,
                               UErrorCode* ec);










U_STABLE double U_EXPORT2
ucurr_getRoundingIncrement(const UChar* currency,
                           UErrorCode* ec);







typedef enum UCurrCurrencyType {
    



    UCURR_ALL = INT32_MAX,
    







    UCURR_COMMON = 1,
    






    UCURR_UNCOMMON = 2,
    




    UCURR_DEPRECATED = 4,
    




    UCURR_NON_DEPRECATED = 8
} UCurrCurrencyType;










U_STABLE UEnumeration * U_EXPORT2
ucurr_openISOCurrencies(uint32_t currType, UErrorCode *pErrorCode);


























 
U_STABLE UBool U_EXPORT2
ucurr_isAvailable(const UChar* isoCode, 
             UDate from, 
             UDate to, 
             UErrorCode* errorCode);















U_STABLE int32_t U_EXPORT2
ucurr_countCurrencies(const char* locale, 
                 UDate date, 
                 UErrorCode* ec); 



















 
U_STABLE int32_t U_EXPORT2 
ucurr_forLocaleAndDate(const char* locale, 
                UDate date, 
                int32_t index,
                UChar* buff, 
                int32_t buffCapacity, 
                UErrorCode* ec); 

















U_STABLE UEnumeration* U_EXPORT2
ucurr_getKeywordValuesForLocale(const char* key,
                                const char* locale,
                                UBool commonlyUsed,
                                UErrorCode* status);

#ifndef U_HIDE_DRAFT_API









U_DRAFT int32_t U_EXPORT2
ucurr_getNumericCode(const UChar* currency);
#endif  

#endif 

#endif
