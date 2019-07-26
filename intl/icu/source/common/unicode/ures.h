





















#ifndef URES_H
#define URES_H

#include "unicode/utypes.h"
#include "unicode/uloc.h"
#include "unicode/localpointer.h"























struct UResourceBundle;




typedef struct UResourceBundle UResourceBundle;






typedef enum {
    
    URES_NONE=-1,

    
    URES_STRING=0,

    
    URES_BINARY=1,

    
    URES_TABLE=2,

    






    URES_ALIAS=3,

    






    URES_INT=7,

    
    URES_ARRAY=8,

    




    URES_INT_VECTOR = 14,
#ifndef U_HIDE_DEPRECATED_API
    
    RES_NONE=URES_NONE,
    
    RES_STRING=URES_STRING,
    
    RES_BINARY=URES_BINARY,
    
    RES_TABLE=URES_TABLE,
    
    RES_ALIAS=URES_ALIAS,
    
    RES_INT=URES_INT,
    
    RES_ARRAY=URES_ARRAY,
    
    RES_INT_VECTOR=URES_INT_VECTOR,
    
    RES_RESERVED=15, 
#endif 

    URES_LIMIT = 16
} UResType;

































U_STABLE UResourceBundle*  U_EXPORT2 
ures_open(const char*    packageName,
          const char*  locale, 
          UErrorCode*     status);



















U_STABLE UResourceBundle* U_EXPORT2 
ures_openDirect(const char* packageName, 
                const char* locale, 
                UErrorCode* status);



















U_STABLE UResourceBundle* U_EXPORT2 
ures_openU(const UChar* packageName, 
           const char* locale, 
           UErrorCode* status);

#ifndef U_HIDE_DEPRECATED_API
















U_DEPRECATED int32_t U_EXPORT2 
ures_countArrayItems(const UResourceBundle* resourceBundle,
                     const char* resourceKey,
                     UErrorCode* err);
#endif  








U_STABLE void U_EXPORT2 
ures_close(UResourceBundle* resourceBundle);

#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN










U_DEFINE_LOCAL_OPEN_POINTER(LocalUResourceBundlePointer, UResourceBundle, ures_close);

U_NAMESPACE_END

#endif

#ifndef U_HIDE_DEPRECATED_API










U_DEPRECATED const char* U_EXPORT2 
ures_getVersionNumber(const UResourceBundle*   resourceBundle);
#endif  










U_STABLE void U_EXPORT2 
ures_getVersion(const UResourceBundle* resB, 
                UVersionInfo versionInfo);

#ifndef U_HIDE_DEPRECATED_API












U_DEPRECATED const char* U_EXPORT2 
ures_getLocale(const UResourceBundle* resourceBundle, 
               UErrorCode* status);
#endif  













U_STABLE const char* U_EXPORT2 
ures_getLocaleByType(const UResourceBundle* resourceBundle, 
                     ULocDataLocaleType type, 
                     UErrorCode* status);


#ifndef U_HIDE_INTERNAL_API
















U_INTERNAL void U_EXPORT2 
ures_openFillIn(UResourceBundle *r, 
                const char* packageName,
                const char* localeID, 
                UErrorCode* status);
#endif  


















U_STABLE const UChar* U_EXPORT2 
ures_getString(const UResourceBundle* resourceBundle, 
               int32_t* len, 
               UErrorCode* status);
















































U_STABLE const char * U_EXPORT2
ures_getUTF8String(const UResourceBundle *resB,
                   char *dest, int32_t *length,
                   UBool forceCopy,
                   UErrorCode *status);


















U_STABLE const uint8_t* U_EXPORT2 
ures_getBinary(const UResourceBundle* resourceBundle, 
               int32_t* len, 
               UErrorCode* status);


















U_STABLE const int32_t* U_EXPORT2 
ures_getIntVector(const UResourceBundle* resourceBundle, 
                  int32_t* len, 
                  UErrorCode* status);

















U_STABLE uint32_t U_EXPORT2 
ures_getUInt(const UResourceBundle* resourceBundle, 
             UErrorCode *status);

















U_STABLE int32_t U_EXPORT2 
ures_getInt(const UResourceBundle* resourceBundle, 
            UErrorCode *status);











U_STABLE int32_t U_EXPORT2 
ures_getSize(const UResourceBundle *resourceBundle);









U_STABLE UResType U_EXPORT2 
ures_getType(const UResourceBundle *resourceBundle);









U_STABLE const char * U_EXPORT2 
ures_getKey(const UResourceBundle *resourceBundle);











U_STABLE void U_EXPORT2 
ures_resetIterator(UResourceBundle *resourceBundle);








U_STABLE UBool U_EXPORT2 
ures_hasNext(const UResourceBundle *resourceBundle);













U_STABLE UResourceBundle* U_EXPORT2 
ures_getNextResource(UResourceBundle *resourceBundle, 
                     UResourceBundle *fillIn, 
                     UErrorCode *status);













U_STABLE const UChar* U_EXPORT2 
ures_getNextString(UResourceBundle *resourceBundle, 
                   int32_t* len, 
                   const char ** key, 
                   UErrorCode *status);













U_STABLE UResourceBundle* U_EXPORT2 
ures_getByIndex(const UResourceBundle *resourceBundle, 
                int32_t indexR, 
                UResourceBundle *fillIn, 
                UErrorCode *status);












U_STABLE const UChar* U_EXPORT2 
ures_getStringByIndex(const UResourceBundle *resourceBundle, 
                      int32_t indexS, 
                      int32_t* len, 
                      UErrorCode *status);

















































U_STABLE const char * U_EXPORT2
ures_getUTF8StringByIndex(const UResourceBundle *resB,
                          int32_t stringIndex,
                          char *dest, int32_t *pLength,
                          UBool forceCopy,
                          UErrorCode *status);













U_STABLE UResourceBundle* U_EXPORT2 
ures_getByKey(const UResourceBundle *resourceBundle, 
              const char* key, 
              UResourceBundle *fillIn, 
              UErrorCode *status);













U_STABLE const UChar* U_EXPORT2 
ures_getStringByKey(const UResourceBundle *resB, 
                    const char* key, 
                    int32_t* len, 
                    UErrorCode *status);



















































U_STABLE const char * U_EXPORT2
ures_getUTF8StringByKey(const UResourceBundle *resB,
                        const char *key,
                        char *dest, int32_t *pLength,
                        UBool forceCopy,
                        UErrorCode *status);

#if U_SHOW_CPLUSPLUS_API
#include "unicode/unistr.h"

U_NAMESPACE_BEGIN











inline UnicodeString 
ures_getUnicodeString(const UResourceBundle *resB, 
                      UErrorCode* status) 
{
    int32_t len = 0;
    const UChar *r = ures_getString(resB, &len, status);
    return UnicodeString(TRUE, r, len);
}











inline UnicodeString 
ures_getNextUnicodeString(UResourceBundle *resB, 
                          const char ** key, 
                          UErrorCode* status) 
{
    int32_t len = 0;
    const UChar* r = ures_getNextString(resB, &len, key, status);
    return UnicodeString(TRUE, r, len);
}










inline UnicodeString 
ures_getUnicodeStringByIndex(const UResourceBundle *resB, 
                             int32_t indexS, 
                             UErrorCode* status) 
{
    int32_t len = 0;
    const UChar* r = ures_getStringByIndex(resB, indexS, &len, status);
    return UnicodeString(TRUE, r, len);
}











inline UnicodeString 
ures_getUnicodeStringByKey(const UResourceBundle *resB, 
                           const char* key, 
                           UErrorCode* status) 
{
    int32_t len = 0;
    const UChar* r = ures_getStringByKey(resB, key, &len, status);
    return UnicodeString(TRUE, r, len);
}

U_NAMESPACE_END

#endif









U_STABLE UEnumeration* U_EXPORT2
ures_openAvailableLocales(const char *packageName, UErrorCode *status);


#endif 

