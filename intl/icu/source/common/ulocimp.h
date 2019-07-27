






#ifndef ULOCIMP_H
#define ULOCIMP_H

#include "unicode/uloc.h"









U_CAPI UEnumeration* U_EXPORT2
uloc_openKeywordList(const char *keywordList, int32_t keywordListSize, UErrorCode* status);





U_CAPI const UChar * U_EXPORT2
uloc_getTableStringWithFallback(
    const char *path,
    const char *locale,
    const char *tableKey,
    const char *subTableKey,
    const char *itemKey,
    int32_t *pLength,
    UErrorCode *pErrorCode);


#define _isIDSeparator(a) (a == '_' || a == '-')

U_CFUNC const char* 
uloc_getCurrentCountryID(const char* oldID);

U_CFUNC const char* 
uloc_getCurrentLanguageID(const char* oldID);

U_CFUNC int32_t
ulocimp_getLanguage(const char *localeID,
                    char *language, int32_t languageCapacity,
                    const char **pEnd);

U_CFUNC int32_t
ulocimp_getScript(const char *localeID,
                   char *script, int32_t scriptCapacity,
                   const char **pEnd);

U_CFUNC int32_t
ulocimp_getCountry(const char *localeID,
                   char *country, int32_t countryCapacity,
                   const char **pEnd);

U_CAPI const char * U_EXPORT2
locale_getKeywordsStart(const char *localeID);


U_CFUNC UBool
ultag_isUnicodeLocaleKey(const char* s, int32_t len);

U_CFUNC UBool
ultag_isUnicodeLocaleType(const char* s, int32_t len);

U_CFUNC const char*
ulocimp_toBcpKey(const char* key);

U_CFUNC const char*
ulocimp_toLegacyKey(const char* key);

U_CFUNC const char*
ulocimp_toBcpType(const char* key, const char* type, UBool* isKnownKey, UBool* isSpecialType);

U_CFUNC const char*
ulocimp_toLegacyType(const char* key, const char* type, UBool* isKnownKey, UBool* isSpecialType);

#endif
