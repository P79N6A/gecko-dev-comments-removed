






#ifndef UPLURALRULES_H
#define UPLURALRULES_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/localpointer.h"




























enum UPluralType {
    



    UPLURAL_TYPE_CARDINAL,
    



    UPLURAL_TYPE_ORDINAL,
    



    UPLURAL_TYPE_COUNT
};



typedef enum UPluralType UPluralType;





struct UPluralRules;
typedef struct UPluralRules UPluralRules;  










U_STABLE UPluralRules* U_EXPORT2
uplrules_open(const char *locale, UErrorCode *status);










U_DRAFT UPluralRules* U_EXPORT2
uplrules_openForType(const char *locale, UPluralType type, UErrorCode *status);






U_STABLE void U_EXPORT2
uplrules_close(UPluralRules *uplrules);


#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN










U_DEFINE_LOCAL_OPEN_POINTER(LocalUPluralRulesPointer, UPluralRules, uplrules_close);

U_NAMESPACE_END

#endif













U_STABLE int32_t U_EXPORT2
uplrules_select(const UPluralRules *uplrules,
               double number,
               UChar *keyword, int32_t capacity,
               UErrorCode *status);

#endif 

#endif
