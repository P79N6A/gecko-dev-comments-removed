






#ifndef __ULDNAMES_H__
#define __ULDNAMES_H__






#include "unicode/utypes.h"
#include "unicode/localpointer.h"
#include "unicode/uscript.h"
#include "unicode/udisplaycontext.h"





typedef enum {
    




    ULDN_STANDARD_NAMES = 0,
    




    ULDN_DIALECT_NAMES
} UDialectHandling;





struct ULocaleDisplayNames;





typedef struct ULocaleDisplayNames ULocaleDisplayNames;  

#if !UCONFIG_NO_FORMATTING













U_STABLE ULocaleDisplayNames * U_EXPORT2
uldn_open(const char * locale,
          UDialectHandling dialectHandling,
          UErrorCode *pErrorCode);






U_STABLE void U_EXPORT2
uldn_close(ULocaleDisplayNames *ldn);

#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN










U_DEFINE_LOCAL_OPEN_POINTER(LocalULocaleDisplayNamesPointer, ULocaleDisplayNames, uldn_close);

U_NAMESPACE_END

#endif










U_STABLE const char * U_EXPORT2
uldn_getLocale(const ULocaleDisplayNames *ldn);







U_STABLE UDialectHandling U_EXPORT2
uldn_getDialectHandling(const ULocaleDisplayNames *ldn);














U_STABLE int32_t U_EXPORT2
uldn_localeDisplayName(const ULocaleDisplayNames *ldn,
                       const char *locale,
                       UChar *result,
                       int32_t maxResultSize,
                       UErrorCode *pErrorCode);














U_STABLE int32_t U_EXPORT2
uldn_languageDisplayName(const ULocaleDisplayNames *ldn,
                         const char *lang,
                         UChar *result,
                         int32_t maxResultSize,
                         UErrorCode *pErrorCode);












U_STABLE int32_t U_EXPORT2
uldn_scriptDisplayName(const ULocaleDisplayNames *ldn,
                       const char *script,
                       UChar *result,
                       int32_t maxResultSize,
                       UErrorCode *pErrorCode);












U_STABLE int32_t U_EXPORT2
uldn_scriptCodeDisplayName(const ULocaleDisplayNames *ldn,
                           UScriptCode scriptCode,
                           UChar *result,
                           int32_t maxResultSize,
                           UErrorCode *pErrorCode);












U_STABLE int32_t U_EXPORT2
uldn_regionDisplayName(const ULocaleDisplayNames *ldn,
                       const char *region,
                       UChar *result,
                       int32_t maxResultSize,
                       UErrorCode *pErrorCode);












U_STABLE int32_t U_EXPORT2
uldn_variantDisplayName(const ULocaleDisplayNames *ldn,
                        const char *variant,
                        UChar *result,
                        int32_t maxResultSize,
                        UErrorCode *pErrorCode);












U_STABLE int32_t U_EXPORT2
uldn_keyDisplayName(const ULocaleDisplayNames *ldn,
                    const char *key,
                    UChar *result,
                    int32_t maxResultSize,
                    UErrorCode *pErrorCode);













U_STABLE int32_t U_EXPORT2
uldn_keyValueDisplayName(const ULocaleDisplayNames *ldn,
                         const char *key,
                         const char *value,
                         UChar *result,
                         int32_t maxResultSize,
                         UErrorCode *pErrorCode);

#ifndef U_HIDE_INTERNAL_API














U_INTERNAL ULocaleDisplayNames * U_EXPORT2
uldn_openForContext(const char * locale, UDisplayContext *contexts,
                    int32_t length, UErrorCode *pErrorCode);











U_INTERNAL UDisplayContext U_EXPORT2
uldn_getContext(const ULocaleDisplayNames *ldn, UDisplayContextType type,
                UErrorCode *pErrorCode);

#endif  

#endif  
#endif  
